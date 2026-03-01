#include <engine/world/world.hpp>
#include <engine/world/prefab.hpp>
#include <engine/render/render_systems.hpp>
#include <engine/services/thread_service.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/animation/module.hpp>
#include <engine/components/sprite.hpp>
#include <engine/components/camera.hpp>
#include <engine/components/transforms.hpp>
#include <core/logging.hpp>
#include <core/hash.hpp>
#include <core/mutex.hpp>
#include <engine.hpp>

namespace kokoro
{
	namespace world
	{
		namespace detail
		{
			//------------------------------------------------------------------------------------------------------------------------
			std::pair<bool, uint64_t> do_check_is_flecs_built_in_phase(std::string_view name)
			{
				static array_t<std::pair<std::string, uint64_t>, 8> C_PHASES =
				{
					std::pair("OnLoad",		(uint64_t)flecs::OnLoad),
					std::pair("PostLoad",	(uint64_t)flecs::PostLoad),
					std::pair("PreUpdate",	(uint64_t)flecs::PreUpdate),
					std::pair("OnUpdate",	(uint64_t)flecs::OnUpdate),
					std::pair("OnValidate", (uint64_t)flecs::OnValidate),
					std::pair("PostUpdate", (uint64_t)flecs::PostUpdate),
					std::pair("PreStore",	(uint64_t)flecs::PreStore),
					std::pair("OnStore",	(uint64_t)flecs::OnStore)
				};

				if (const auto& it = algorithm::find_if(C_PHASES.begin(), C_PHASES.end(), [=](const auto& pair)
					{
						return string_utils::compare(pair.first, name.data());

					}); it != C_PHASES.end())
				{
					return { true, (uint64_t)it->second };
				}

				return { false, MAXIMUM(uint64_t) };
			}

			//------------------------------------------------------------------------------------------------------------------------
			void world_to_scene_entity_recursively(const sworld& w, flecs::entity e, sscene::entities_t& entities)
			{
				const auto* id = e.get<components::sidentifier>();

				//- Serialize entity basic information
				auto& se = entities.emplace_back();
				se.m_name = id->m_name;
				se.m_uuid = id->m_uuid.string();

				//- Serialize entity components
				for (const auto& c : w.m_entity_comps.at(e.id()))
				{
					auto& sc = se.m_comps.emplace_back();

					auto type = rttr::type::get_by_name(c);

					sc.m_data = rttr::detail::invoke_static_function(type, "get", e);
					sc.m_type_name = type.get_name().data();
				}

				//- Serialize recursively children of the entity
				const auto* hier = e.get<components::shierarchy>();

				for (const auto& uuid : hier->m_children)
				{
					if (auto kid = entity::find(w, uuid.string()); kid.is_valid())
					{
						world_to_scene_entity_recursively(w, kid, se.m_kids);
					}
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			sscene world_to_scene(const sworld& w)
			{
				sscene scene;
				scene.m_name = w.name();

				//- Find entry-point entities that dont have parents and only children
				std::vector<flecs::entity> entries;
				for (const auto& e : w.m_all_entities)
				{
					const auto* hier = e.get<components::shierarchy>();
					if (hier->m_parent == core::cuuid::C_INVALID_UUID)
					{
						entries.emplace_back(e);
					}
				}

				//- For each of the entry point entities, recursively serialize their data
				//- into the scene format
				for (const auto& e : entries)
				{
					//- FIXME: what if the entity is the root of a prefab?
					//- Serialize only if it has some overrides or something (for comps that are not transforms)
					//- otherwise serialize a filepath to the prefab or something...
					world_to_scene_entity_recursively(w, e, scene.m_entities);
				}

				return scene;
			}

		} //- detail

		namespace
		{
			//------------------------------------------------------------------------------------------------------------------------
			struct simported_system final
			{
				std::string m_name;
				vector_t<std::string> m_run_before;
				vector_t<std::string> m_run_after;
			};

			//------------------------------------------------------------------------------------------------------------------------
			struct simported_module final
			{
				using comps_t = std::vector<std::string>;
				using systems_t = std::vector<simported_system>;
				using deps_t = std::vector<std::string>;

				std::string m_name;
				deps_t m_deps;
				systems_t m_systems;
				comps_t m_comps;
			};

			core::cmutex mutex;
			std::unordered_map<uint64_t, sworld> cache;
			std::unordered_map<int, fs::cfilepath> paths;
			uint64_t current_active_world = MAXIMUM(uint64_t);

			//------------------------------------------------------------------------------------------------------------------------
			kokoro::modules::sconfig get_module_config(std::string_view type)
			{
				if (const auto t = rttr::type::get_by_name(type.data()); t.is_valid())
				{
					if (const auto m = t.get_method(kokoro::modules::smodule::C_MODULE_CONFIG_FUNC_NAME); m.is_valid())
					{
						return m.invoke({}).convert<kokoro::modules::sconfig>();
					}
				}
				return {};
			}

			//------------------------------------------------------------------------------------------------------------------------
			kokoro::system::sconfig get_system_config(std::string_view type)
			{
				if (const auto t = rttr::type::get_by_name(type.data()); t.is_valid())
				{
					if (const auto m = t.get_method(kokoro::system::ssystem::C_SYSTEM_CONFIG_FUNC_NAME); m.is_valid())
					{
						return m.invoke({}).convert<kokoro::system::sconfig>();
					}
				}
				return {};
			}

			//------------------------------------------------------------------------------------------------------------------------
			core::plugin::sconfig get_plugin_config(std::string_view type)
			{
				if (const auto t = rttr::type::get_by_name(type.data()); t.is_valid())
				{
					if (const auto m = t.get_method(core::plugin::C_PLUGIN_CONFIG_FUNC_NAME); m.is_valid())
					{
						return m.invoke({}).convert<core::plugin::sconfig>();
					}
				}
				return {};
			}

			//- Function takes in raw modules to be imported and creates a sorted order of import. We order by explicit module to module
			//- dependency and implicit module-system to module dependencies.
			//------------------------------------------------------------------------------------------------------------------------
			std::vector<std::string> topsort(const std::vector<simported_module>& data, bool verbose)
			{
				std::unordered_map<std::string, uset_t<std::string>> systems_module_map; //- Mapping all systems to their dependency modules
				std::unordered_map<std::string, uset_t<std::string>> modules_to_module_map; //- Mapping all modules to their dependency modules

				//- First of all, create the system-to-module map required for lookup of relationships
				for (const auto& d : data)
				{
					for (const auto& s : d.m_systems)
					{
						systems_module_map[s.m_name].insert(d.m_name);
					}
				}

				for (auto& [s, m] : systems_module_map)
				{
					const auto cfg = get_system_config(s);

					for (auto& rb : cfg.m_run_before)
					{
						if (const auto [result, _] = detail::do_check_is_flecs_built_in_phase(rb); result)
						{
							continue;
						}

						auto& modules = systems_module_map.at(rb);

						m.insert(modules.begin(), modules.end());
					}

					for (auto& ra : cfg.m_run_after)
					{
						if (const auto [result, _] = detail::do_check_is_flecs_built_in_phase(ra); result)
						{
							continue;
						}

						auto& modules = systems_module_map.at(ra);

						m.insert(modules.begin(), modules.end());
					}
				}

				//- Establish module-to-module dependencies, first the explicit and afterwards the implicit defined through system
				//- dependencies
				for (const auto& d : data)
				{
					auto& deps = modules_to_module_map[d.m_name];

					for (const auto& m : d.m_deps)
					{
						//- Explicit dependency defined for module
						deps.insert(m);
					}

					for (const auto& s : d.m_systems)
					{
						const auto& system_modules = systems_module_map.at(s.m_name);

						//- Implicit dependency induced through this modules system dependencies
						for (const auto& dep : system_modules)
						{
							deps.insert(dep);
						}
					}
				}

				//- Cleanup introduced self-dependency
				for (auto& [m, d] : modules_to_module_map)
				{
					d.erase(m);
				}

				const auto do_topsort = [](const umap_t<std::string, uset_t<std::string>>& graph) -> vector_t<std::string>
					{
						vector_t<std::string> sorted;
						uset_t<std::string> visited;
						uset_t<std::string> recursion_stack;

						std::function<bool(const std::string&)> depth_first_search = [&](const std::string& node) -> bool
							{
								if (recursion_stack.count(node))
								{
									return true; // cycle detected
								}

								if (visited.count(node))
								{
									return false; // already processed
								}

								visited.insert(node);
								recursion_stack.insert(node);

								if (const auto it = graph.find(node); it != graph.end())
								{
									for (const auto& dep : it->second)
									{
										if (depth_first_search(dep))
										{
											return true; // propagate cycle detection
										}
									}
								}

								recursion_stack.erase(node);
								sorted.push_back(node);
								return false;
							};


						for (const auto& [module, _] : graph)
						{
							if (!visited.count(module))
							{
								if (depth_first_search(module))
								{
									log_critical(fmt::format("Cyclic module dependency '{}'", module));
									CORE_ASSERT(false, "Cyclic module dependency detected!");
									return {};
								}
							}
						}

						return sorted;
					};

				auto result = do_topsort(modules_to_module_map);

				if (verbose)
				{
					log_debug(fmt::format("[Module Manager] Imported systems '{}' and imported modules '{}'", systems_module_map.size(), modules_to_module_map.size()));
					log_debug("[Module Manager] System-to-module dependencies:");
					for (const auto& [system, deps] : systems_module_map)
					{
						log_debug(fmt::format("\t{}", system));
						log_debug(fmt::format("\t\tdeps modules: {}", string_utils::join(deps.begin(), deps.end(), ", ")));
					}

					log_debug("[Module Manager] Module-to-module dependencies:");
					for (const auto& [module, deps] : modules_to_module_map)
					{
						log_debug(fmt::format("\t{}", module));
						log_debug(fmt::format("\t\tdeps modules: {}", string_utils::join(deps.begin(), deps.end(), ", ")));
					}

					log_debug("[Module Manager] Resulting import order:");
					for (const auto& module : result)
					{
						log_debug(fmt::format("\t{}", module));
						for (auto s : get_module_config(module).m_systems)
						{
							log_debug(fmt::format("\t\t{}", s));
						}
					}
				}

				return result;
			}

			//------------------------------------------------------------------------------------------------------------------------
			query_t next_query_key(sworld& w)
			{
				return (w.m_next_query_key + 1) % sworld::C_MASTER_QUERY_KEY_MAX;
			}

			//------------------------------------------------------------------------------------------------------------------------
			squery::result_t result_type(squery::type t)
			{
				switch (t)
				{
				case squery::type_count:	return rttr::variant(static_cast<uint64_t>(0));
				case squery::type_any:		return rttr::variant(false);
				case squery::type_all:		return rttr::variant(std::vector<flecs::entity>{});
				default:
				case squery::type_none: CORE_ASSERT(false, "Unrecognized query type"); return {};
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			void do_query(sworld& w, const squery& q)
			{
				w.m_query_results[w.m_current_query_key] = result_type(q.m_type);
				auto& tree = w.m_proxy_manager.m_quad_tree;

				if (w.m_current_query.m_intersection == squery::intersection_aabb)
				{
					CORE_ASSERT(q.m_data.get_type() == rttr::type::get<aabb_t>(),
						"Invalid operation. Unexpected config data type for aabb intersection");

					box2d::b2DynamicTree_Query(&tree, q.m_data.get_value<aabb_t>(), B2_DEFAULT_MASK_BITS, &query::detail::query_callback, &w);
				}

				else if (w.m_current_query.m_intersection == squery::intersection_ray)
				{
					CORE_ASSERT(q.m_data.get_type() == rttr::type::get<ray_t>(),
						"Invalid operation. Unexpected config data type for ray intersection");

					box2d::b2RayCastInput ray = q.m_data.get_value<ray_t>();

					box2d::b2DynamicTree_RayCast(&tree, &ray, B2_DEFAULT_MASK_BITS, &query::detail::raycast_callback, &w);
				}
				else
				{
					CORE_ASSERT(false, "Unrecognized query intersection type");
				}
			}

		} //- unnamed

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::sproxy_manager::tick()
		{
			CORE_ZONE;

			//- process proxy creation requests
			for (const auto& e : m_creation_queue)
			{
				create_proxy(e);
			}
			m_creation_queue.clear();
		}

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::sproxy_manager::update_proxy(flecs::entity e)
		{
			CORE_ZONE;

			if (const auto* id = e.get<components::sidentifier>(); id)
			{
				if (const auto it = m_proxies.find(id->m_uuid); it != m_proxies.end())
				{
					auto& proxy = m_proxies[id->m_uuid];
					const auto* sprite = e.get<render::component::ssprite>();
					const auto* transform = e.get<render::component::sworld_transform>();
					proxy.m_aabb = transform->m_aabb;
					box2d::b2DynamicTree_MoveProxy(&m_quad_tree, proxy.m_id, transform->m_aabb);

					log_debug(fmt::format("Updating proxy '{} ({})'", id->m_name, id->m_uuid.string()));
				}
				else
				{
					log_error(fmt::format("Trying to update unknown proxy '{}'", e.id()));
				}
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::sproxy_manager::destroy_proxy(flecs::entity e)
		{
			CORE_ZONE;

			if (const auto* id = e.get<components::sidentifier>(); id)
			{
				if (const auto it = m_proxies.find(id->m_uuid); it != m_proxies.end())
				{
					box2d::b2DynamicTree_DestroyProxy(&m_quad_tree, it->second.m_id);

					algorithm::erase_at(m_proxies, it);

					log_debug(fmt::format("Destroying proxy '{} ({})'", id->m_name, id->m_uuid.string()));
				}
			}
			else
			{
				log_error(fmt::format("Trying to destroy unknown proxy '{}'", e.id()));
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::sproxy_manager::create_proxy(flecs::entity e)
		{
			const auto* id = e.get<components::sidentifier>();
			const auto* sprite = e.get<render::component::ssprite>();
			const auto* transform = e.get<render::component::sworld_transform>();
			const auto data_id = user_data_id(e);
			auto& proxy = m_proxies[id->m_uuid];
			proxy.m_aabb = transform->m_aabb;
			proxy.m_entity = e;
			proxy.m_id = box2d::b2DynamicTree_CreateProxy(&m_quad_tree, transform->m_aabb, B2_DEFAULT_CATEGORY_BITS, data_id);

			m_user_data[data_id] = id->m_uuid;

			log_debug(fmt::format("Creating proxy '{} ({})'", id->m_name, id->m_uuid.string()));
		}

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::sproxy_manager::queue_create_proxy(flecs::entity e)
		{
			log_debug(fmt::format("Queue creating proxy '{}'", e.id()));
			m_creation_queue.push_back(e);
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool sworld::sproxy_manager::has_proxy(flecs::entity e) const
		{
			if (const auto* id = e.get<components::sidentifier>(); id)
			{
				return m_proxies.find(id->m_uuid) != m_proxies.end();
			}
			return false;
		}

		//------------------------------------------------------------------------------------------------------------------------
		sproxy& sworld::sproxy_manager::proxy(int proxy_id)
		{
			if (const auto it = m_user_data.find(proxy_id); it != m_user_data.end())
			{
				if (const auto p = m_proxies.find(it->second); p != m_proxies.end())
				{
					return p->second;
				}
			}

			CORE_ASSERT(false, "Entity with given uuid hash does not exist!");
			static sproxy S_DUMMY;
			return S_DUMMY;
		}

		//------------------------------------------------------------------------------------------------------------------------
		int sworld::sproxy_manager::user_data_id(const flecs::entity& e) const
		{
			return algorithm::hash(e.get<components::sidentifier>()->m_uuid.string());
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld::sworld(std::string_view name) :
			sworld(name, {})
		{
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld::sworld(std::string_view name, sconfig cfg)
			: m_name(name.data()), m_id(algorithm::hash(m_name)), m_cfg(cfg)
		{
			log_debug(fmt::format("Creating world '{}'", this->name()));

			m_proxy_manager.m_quad_tree = box2d::b2DynamicTree_Create();

			auto& w = m_world;
			w.set_ctx(this);
			w.component<components::sidentifier>();
			w.component<components::shierarchy>();
			w.component<components::sprefab>();
			w.component<components::tag::sentity>();
			w.component<components::tag::sinvisible>();
			w.component<components::tag::ssystem>();
			w.component<render::component::srendercamera>();
			w.component<render::component::ssprite>();
			w.component<render::component::slocal_transform>();
			w.component<render::component::sworld_transform>();

			m_proxy_observer = w
				.observer<render::component::ssprite, render::component::slocal_transform, components::sidentifier>()
				.event(flecs::OnAdd)
				.event(flecs::OnRemove)
				.ctx(this)
				.each([](flecs::iter& it, size_t i,
					render::component::ssprite&,
					render::component::slocal_transform&,
					components::sidentifier& id)
					{
						if (auto* world = reinterpret_cast<sworld*>(it.ctx()); world)
						{
							auto e = it.entity(i);
							const auto has_proxy = world->m_proxy_manager.has_proxy(e);

							//- Observe when entities enter or leave the archetype (sprite, transform and id)
							//- and manage their internal dynamic tree proxy
							if (it.event() == flecs::OnAdd && !has_proxy)
							{
								world->m_proxy_manager.queue_create_proxy(e);
							}
							else if (it.event() == flecs::OnRemove && has_proxy)
							{
								world->m_proxy_manager.destroy_proxy(e);
							}
						}
					});

			m_comps_observer = w
				.observer<>()
				.event(flecs::OnAdd)
				.event(flecs::OnRemove)
				.with(flecs::Wildcard)
				.ctx(this)
				.each([](flecs::iter& it, size_t i)
					{
						if (auto* world = reinterpret_cast<sworld*>(it.ctx()); world)
						{
							auto entity = it.entity(i);
							std::string string = it.event_id().str().c_str();

							//- Tag components are required to be inside the 'tag' namespace,
							//- thus when we strip the name we must retain 'tag::' for those components,
							//- otherwise the engine will not recognize them as correct components.
							if (const auto last = string.find_last_of("."); last != std::string::npos)
							{
								const auto is_tag = string_utils::find_substr(string, "tag.") != std::string::npos;
								auto c = string.substr(last + 1);
								if (is_tag) { c = fmt::format("tag::{}", c); }

								if (it.event() == flecs::OnAdd)
								{
									log_debugf("Component added '{}({})' to entity '{}'",
										c, string, entity.id());

									//- name of the component will be in form 'ecs.transform', we have to format the name first
									world->m_entity_comps[entity].insert(c);
								}
								else if (it.event() == flecs::OnRemove)
								{
									log_debugf("Component removed '{}({})' from entity '{}'",
										c, string, entity.id());

									world->m_entity_comps[entity].erase(c);
								}
							}
						}
					});

			m_transform_tracker = query::change_tracker<render::component::slocal_transform>(*this);
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld::~sworld()
		{
			log_debug(fmt::format("Freeing world '{}'", this->name()));

			box2d::b2DynamicTree_Destroy(&m_proxy_manager.m_quad_tree);
			m_proxy_observer.destruct();
			m_comps_observer.destruct();
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool sworld::foreground() const
		{
			return algorithm::bit_check(m_cfg.m_flags, world_flag_foreground) && id() == current_active_world;
		}

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::flags(world_flags_t value)
		{
			m_cfg.m_flags = value;
		}

		namespace components
		{
			//------------------------------------------------------------------------------------------------------------------------
			bool sidentifier::show_ui(editor::ceditor_context_holder*, rttr::variant& comp)
			{
				CORE_ASSERT(comp.get_type() == rttr::type::get<sidentifier>(), "Invalid type");
				return false;
			}

			//------------------------------------------------------------------------------------------------------------------------
			bool shierarchy::show_ui(editor::ceditor_context_holder* ec, rttr::variant& comp)
			{
				CORE_ASSERT(comp.get_type() == rttr::type::get<shierarchy>(), "Invalid type");
				return false;
			}

			//------------------------------------------------------------------------------------------------------------------------
			bool sprefab::show_ui(editor::ceditor_context_holder* ec, rttr::variant& comp)
			{
				CORE_ASSERT(comp.get_type() == rttr::type::get<sprefab>(), "Invalid type");
				return false;
			}

			//------------------------------------------------------------------------------------------------------------------------
			sworld::component_types_t registered(const sworld& w)
			{
				return w.m_all_comps;
			}

			//------------------------------------------------------------------------------------------------------------------------
			sworld::active_component_count_t active(const sworld& w)
			{
				return w.m_active_comps;
			}

			//------------------------------------------------------------------------------------------------------------------------
			sworld::component_types_t all(const sworld& w, flecs::entity e)
			{
				if (const auto it = w.m_entity_comps.find(e.id()); it != w.m_entity_comps.end())
				{
					return it->second;
				}
				return {};
			}

		} //- components

		namespace plugins
		{
			//------------------------------------------------------------------------------------------------------------------------
			void import(sworld& w, std::vector<std::string> plugins)
			{
				for (const auto& p : plugins)
				{
					auto cfg = get_plugin_config(p);

					//- Import dependency plugins
					{
						std::vector<std::string> deps; deps.reserve(cfg.m_plugins.size());
						for (const auto& d : cfg.m_plugins)
						{
							if (const auto t = rttr::type::get_by_name(d); t.is_valid())
							{
								deps.emplace_back(d);
							}
							else
							{
								log_error(fmt::format("Failed adding dependency plugin '{}' for world '{}' and plugin '{}', as the plugin is not reflected to RTTR!",
									p.data(), w.name(), d.data()));
							}
						}

						if (!deps.empty())
						{
							import(w, deps);
						}
					}

					//- Load modules required for plugin
					{
						std::vector<std::string> mods; mods.reserve(cfg.m_modules.size());
						for (const auto& m : cfg.m_modules)
						{
							if (const auto t = rttr::type::get_by_name(m); t.is_valid())
							{
								mods.emplace_back(m);
							}
							else
							{
								log_error(fmt::format("Failed creating module '{}' for world '{}' and plugin '{}', as the plugin is not reflected to RTTR!",
									p.data(), w.name(), m.data()));
							}
						}

						modules::import(w, mods);
					}


				}
			}

		} //- plugins

		namespace modules
		{
			//------------------------------------------------------------------------------------------------------------------------
			void import(sworld& w, std::vector<std::string> modules)
			{
				CORE_ZONE;

				std::vector<simported_module> unsorted_modules;
				std::vector<std::string> sorted_modules;

				//- Transform raw data to a form that we can sort and reason about
				{
					for (const auto& m : modules)
					{
						auto config = get_module_config(m);

						if (config.m_name.empty())
						{
							log_error(fmt::format("Failed to import an unreflected module of type '{}'!", m));
							return;
						}

						//- Do not process module if already imported
						if (const auto it = algorithm::find_if(unsorted_modules.begin(), unsorted_modules.end(),
							[&](const auto& m)
							{
								return m.m_name == config.m_name;

							}); it != unsorted_modules.end())
						{
							return;
						}

						//- Add module to the ones to be loaded without further processing
						auto& m = unsorted_modules.emplace_back();
						m.m_name = config.m_name;

						for (const auto& sys : config.m_systems)
						{
							const auto system_config = get_system_config(sys);

							auto& entry = m.m_systems.emplace_back();
							entry.m_name = sys;
							entry.m_run_after = system_config.m_run_after;
							entry.m_run_before = system_config.m_run_before;
						}

						for (const auto& comp : config.m_components)
						{
							m.m_comps.emplace_back(comp);
						}

						for (const auto& module : config.m_modules)
						{
							m.m_deps.emplace_back(module);
						}
					}
				}

				//- Sort the modules for actual importing
				{
					sorted_modules = topsort(unsorted_modules, true);
				}

				//- Finally import each of the modules
				{
					for (const auto& module : sorted_modules)
					{
						const auto cfg = get_module_config(module);


						auto type = rttr::type::get_by_name(module.data());

						auto var = type.create({ &w });
					}
				}
			}

			//- Function responsible for creating a system for a world with given configuration, without matching any components
			//- and function to execute.
			//- A task is similar to a normal system, only that it does not match any components and thus no entities.
			//- If entities are required they can be retrieved through the world or a query.
			//- The function itself is executed as is, with only delta time provided.
			//------------------------------------------------------------------------------------------------------------------------
			void create_task(sworld& w, const kokoro::system::sconfig& cfg, kokoro::system::task_callback_t callback)
			{
				CORE_ASSERT(!(algorithm::bit_check(cfg.m_flags, system::system_flag_multithreaded) &&
					algorithm::bit_check(cfg.m_flags, system::system_flag_immediate)), "A system cannot be multithreaded and immediate at the same time!");

				auto builder = w.m_world.system(cfg.m_name.c_str());

				//- Set options that are required before system entity creation
				{
					if (algorithm::bit_check(cfg.m_flags, system::system_flag_multithreaded))
					{
						builder.multi_threaded();
					}
					else if (algorithm::bit_check(cfg.m_flags, system::system_flag_immediate))
					{
						builder.immediate();
					}
				}

				if (const auto duplicate_system = find_system(w, cfg.m_name); duplicate_system)
				{
					log_warn(fmt::format("Trying to create a system with same name twice '{}'. This is not allowed!", cfg.m_name));
					return;
				}

				if (cfg.m_interval != 0)
				{
					builder.interval(cfg.m_interval);
				}

				//- Create function to be called for running the task
				auto function = [=](flecs::iter& it)
					{
						(callback)(it, it.delta_time());
					};

				auto task = builder.run(std::move(function));

				task.add<components::tag::ssystem>();

				//- Set options that are required after system entity creation
				{
					for (const auto& after : cfg.m_run_after)
					{
						if (const auto [result, phase] = detail::do_check_is_flecs_built_in_phase(after); result)
						{
							task.add(flecs::Phase).depends_on(phase);
						}
						else
						{
							if (auto e = find_system(w, after); e)
							{
								task.add(flecs::Phase).depends_on(e);
							}
							else
							{
								log_error(fmt::format("Dependency (run after) system '{}' for system '{}' could not be found!",
									after, cfg.m_name));
							}
						}
					}

					for (const auto& before : cfg.m_run_before)
					{
						if (const auto [result, phase] = detail::do_check_is_flecs_built_in_phase(before); result)
						{
							task.add(flecs::Phase).depends_on(phase);
						}
						else
						{
							if (auto e = find_system(w, before); e)
							{
								e.add(flecs::Phase).depends_on(task);
							}
							else
							{
								log_error(fmt::format("Dependent (run before) system '{}' for system '{}' could not be found!",
									before, cfg.m_name));
							}
						}
					}
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			flecs::system find_system(const sworld& w, std::string_view name)
			{
				if (const auto it = w.m_all_systems.find(name.data()); it != w.m_all_systems.end())
				{
					return it->second;
				}
				return flecs::system{};
			}

		} //- modules

		namespace singletons
		{
			//------------------------------------------------------------------------------------------------------------------------
			sworld::component_types_t registered(const sworld& w)
			{
				return w.m_all_singletons;
			}

			//------------------------------------------------------------------------------------------------------------------------
			sworld::component_types_t all(const sworld& w)
			{
				return w.m_active_singletons;
			}

			//------------------------------------------------------------------------------------------------------------------------
			void add(sworld& w, const rttr::variant& c)
			{
				rttr::detail::invoke_static_function(c.get_type(), kokoro::ecs::detail::C_COMPONENT_ADD_SINGLETON_FUNC_NAME, w.m_world);
				w.m_active_singletons.insert(c.get_type().get_name().data());

			}

			//------------------------------------------------------------------------------------------------------------------------
			void remove(sworld& w, const rttr::type& t)
			{
				rttr::detail::invoke_static_function(t, kokoro::ecs::detail::C_COMPONENT_REMOVE_SINGLETON_FUNC_NAME, w.m_world);
				w.m_active_singletons.erase(t.get_name().data());
			}

		} //- singletons

		namespace entity
		{
			//------------------------------------------------------------------------------------------------------------------------
			void add(sworld& w, flecs::entity e, const rttr::variant& c)
			{
				rttr::detail::invoke_static_function(c.get_type(), kokoro::ecs::detail::C_COMPONENT_SET_FUNC_NAME, e, c);
				w.m_entity_comps[e.id()].insert(c.get_type().get_name().data());
			}

			//------------------------------------------------------------------------------------------------------------------------
			void remove(sworld& w, flecs::entity e, const rttr::type& t)
			{
				rttr::detail::invoke_static_function(t, kokoro::ecs::detail::C_COMPONENT_REMOVE_FUNC_NAME, e);

				auto& comps = w.m_entity_comps[e.id()];
				if (const auto it = algorithm::find_at(comps.begin(), comps.end(), t.get_name().data()); it != comps.end())
				{
					algorithm::erase_at(comps, it);
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			flecs::entity create(sworld& w, std::string_view name, std::string_view uuid /*= {}*/,
				std::string_view parent /*= {}*/, bool is_prefab /*= false*/)
			{
				CORE_ASSERT(!name.empty(), "Invalid operation. Cannot set empty name for entity creation!");

				flecs::entity out = w.m_world.entity(name.data());

				if (!is_prefab)
				{
					add<components::tag::sentity>(w, out);
				}
				else
				{
					add<components::sprefab>(w, out);
				}

				add<components::sidentifier>(w, out);
				add<components::shierarchy>(w, out);
				add<render::component::sworld_transform>(w, out);
				add<render::component::slocal_transform>(w, out);

				//- Identifier component holding the unique entity uuid and a name
				auto* id = out.get_mut<components::sidentifier>();
				id->m_name = name.data();
				id->m_uuid = uuid.empty() ? core::cuuid().string() : std::string(uuid.data());
				out.set_alias(id->m_uuid.string().data());

				if (!parent.empty())
				{
					if (auto e_parent = find(const_cast<const sworld&>(w), parent); e_parent.is_valid())
					{
						const auto* p_identifier = e_parent.get<components::sidentifier>();
						auto* p_hierarchy = e_parent.get_mut<components::shierarchy>();

						auto* hierarchy = out.get_mut<components::shierarchy>();
						hierarchy->m_parent = p_identifier->m_uuid;
						p_hierarchy->m_children.emplace_back(id->m_uuid);
						out.child_of(e_parent);
					}
				}

				//- Track created entity and create a proxy too
				w.m_all_entities.emplace_back(out);
				w.m_proxy_manager.create_proxy(out);

				return out;
			}

			//------------------------------------------------------------------------------------------------------------------------
			flecs::entity find(const sworld& w, std::string_view name_or_uuid)
			{
				return w.m_world.lookup(name_or_uuid.data());
			}

			//------------------------------------------------------------------------------------------------------------------------
			void kill(sworld& w, std::string_view name_or_uuid)
			{
				if (auto e = find(w, name_or_uuid); e.is_valid())
				{
					kill(w, e);
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			void kill(sworld& w, const sworld::entities_t& entities)
			{
				for (const auto& e : entities)
				{
					kill(w, e);
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			void kill(sworld& w, flecs::entity e)
			{
				const auto* id = e.get<components::sidentifier>();
				const auto* hierarchy = e.get<components::shierarchy>();

				//- Copy array of children, as we wil erase entries from hierarchy children array recursively
				auto children = hierarchy->m_children;
				for (const auto& kid : children)
				{
					kill(w, kid.string());
				}

				//- Erase kid from parents children array if it has one
				if (hierarchy->m_parent != core::cuuid::C_INVALID_UUID)
				{
					if (auto e_parent = find(w, hierarchy->m_parent.string()); e_parent.is_valid())
					{
						auto* hierarchy = e_parent.get_mut<components::shierarchy>();
						const auto s = id->m_uuid.string();
						algorithm::erase_if(hierarchy->m_children, [&](const auto& k)
							{
								return k.string() == s;
							});
					}
				}

				//- Destroy entity within ecs world to free components and data
				e.destruct();

				//- Untrack entity and destroy its proxy
				if (const auto it = algorithm::find_at(w.m_all_entities.begin(), w.m_all_entities.end(), e); it != w.m_all_entities.end())
				{
					algorithm::erase_at(w.m_all_entities, it);
				}

				w.m_proxy_manager.destroy_proxy(e);
			}

		} //- entity

		namespace query
		{
			//------------------------------------------------------------------------------------------------------------------------
			query_t create(sworld& w, squery::type type, squery::intersection intersection, squery::data_t data,
				squery::filters_t filters /*= {}*/)
			{
				query_t i = w.m_next_query_key;
				w.m_next_query_key = next_query_key(w);

				squery q;
				q.m_type = type;
				q.m_intersection = intersection;
				q.m_data = data;
				q.m_filters = filters;

				w.m_queries.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(i),
					std::forward_as_tuple(std::move(q)));

				return i;
			}

			//------------------------------------------------------------------------------------------------------------------------
			squery::result_t immediate(sworld& w, squery::type type, squery::intersection intersection, squery::data_t data,
				squery::filters_t filters /*= {}*/)
			{
				squery q;
				q.m_type = type;
				q.m_intersection = intersection;
				q.m_data = data;
				q.m_filters = filters;

				query_t i = w.m_next_query_key;
				w.m_next_query_key = next_query_key(w);

				w.m_current_query_key = i;
				w.m_current_query = q;
				w.m_query_results[i] = result_type(q.m_type);

				do_query(w, q);

				//- Get the result for return and erase it from result map
				const auto it = w.m_query_results.find(i);
				rttr::variant var = std::move(it->second);
				algorithm::erase_at(w.m_query_results, it);

				return var;
			}

			//------------------------------------------------------------------------------------------------------------------------
			std::optional<squery::result_t> result(sworld& w, query_t id)
			{
				std::optional<squery::result_t> out = std::nullopt;

				if (const auto it = w.m_query_results.find(id); it != w.m_query_results.end())
				{
					out = it->second;

					//- Result will be taken and we do not need to hold it anymore
					algorithm::erase_at(w.m_query_results, it);
				}
				return out;
			}

			namespace detail
			{
				//------------------------------------------------------------------------------------------------------------------------
				bool query_callback(int proxy_id, int user_data, void* ctx)
				{
					CORE_ZONE;

					auto* w = reinterpret_cast<sworld*>(ctx);
					auto& pm = w->m_proxy_manager;
					bool result = false;

					switch (w->m_current_query.m_type)
					{
					case squery::type_count: { result = true; break; }
					case squery::type_all: { result = true; break; }
					case squery::type_any: { result = false; break; }
					default:
					case squery::type_none: return false;
					}

					//- Assign query key to proxy and avaoid duplicate queries
					auto& proxy = pm.proxy(user_data/*box2d::b2DynamicTree_GetUserData(&pm.tree(), proxy_id)*/);

					CORE_ASSERT(proxy.m_entity.id() != 0, "Invalid entity inside dynamic tree");

					if (proxy.m_query_key == w->m_current_query_key)
					{
						return result;
					}

					proxy.m_query_key = w->m_current_query_key;

					//- TODO: apply filters

					switch (w->m_current_query.m_type)
					{
					case squery::type_count:
					{
						auto& var = w->m_query_results[w->m_current_query_key];
						auto& value = var.get_value<uint64_t>(); ++value;
						break;
					}
					case squery::type_all:
					{
						auto& var = w->m_query_results[w->m_current_query_key];
						auto& value = var.get_value<std::vector<flecs::entity>>();
						value.push_back(proxy.m_entity);
						break;
					}
					case squery::type_any:
					{
						auto& var = w->m_query_results[w->m_current_query_key];
						auto& value = var.get_value<bool>();
						value = true;
						break;
					}
					default:
					case squery::type_none: return false;
					}

					return result;
				}

				//------------------------------------------------------------------------------------------------------------------------
				float raycast_callback(const box2d::b2RayCastInput* input, int proxy_id, int user_data, void* ctx)
				{
					CORE_ZONE;

					//- 0.0f signals to stop and 1.0f signals to continue
					float result = 0.0f;
					auto* w = reinterpret_cast<sworld*>(ctx);
					auto& pm = w->m_proxy_manager;

					switch (w->m_current_query.m_type)
					{
					case squery::type_count: { result = 1.0f; break; }
					case squery::type_all: { result = 1.0f; break; }
					case squery::type_any: { result = 0.0f; break; }
					default:
					case squery::type_none: return false;
					}

					//- Assign query key to proxy and avaoid duplicate queries
					auto& proxy = pm.proxy(user_data/*box2d::b2DynamicTree_GetUserData(&pm.tree(), proxy_id)*/);

					CORE_ASSERT(proxy.m_entity.id() != 0, "Invalid entity inside dynamic tree");

					if (proxy.m_query_key == w->m_current_query_key)
					{
						return result;
					}

					proxy.m_query_key = w->m_current_query_key;

					//- TODO: apply filters

					switch (w->m_current_query.m_type)
					{
					case squery::type_count:
					{
						auto& var = w->m_query_results[w->m_current_query_key];
						auto& value = var.get_value<uint64_t>(); ++value;
						break;
					}
					case squery::type_all:
					{
						auto& var = w->m_query_results[w->m_current_query_key];
						auto& value = var.get_value<std::vector<flecs::entity>>();
						value.emplace_back(proxy.m_entity);
						break;
					}
					case squery::type_any:
					{
						auto& var = w->m_query_results[w->m_current_query_key];
						auto& value = var.get_value<bool>();
						value = true;
						break;
					}
					default:
					case squery::type_none: return false;
					}

					return result;
				}

			} //- detail

		} //- query

		//------------------------------------------------------------------------------------------------------------------------
		entity_set_t visible_entities(const sworld& w, const aabb_t& area)
		{
			if (const auto result = query::immediate(const_cast<sworld&>(w),
				squery::type_all,
				squery::intersection_aabb,
				area); result.is_valid())
			{
				return result.get_value<entity_set_t>();
			}
			return {};
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld::entities_t entities(const sworld& w)
		{
			return w.m_all_entities;
		}

		//------------------------------------------------------------------------------------------------------------------------
		void tick(sworld& w, float dt)
		{
			CORE_ZONE;

			for (auto& [_, tracker] : w.m_change_trackers)
			{
				tracker->tick();
			}

			w.m_world.progress(dt);

			if (auto t = w.m_transform_tracker.lock(); t)
			{
				t->tick();
			}

			//- Clear previous results so that we only contain fresh ones
			w.m_query_results.clear();

			for (auto& [key, cfg] : w.m_queries)
			{
				//- Prepare query and let it run
				w.m_current_query_key = key;
				w.m_current_query = cfg;
				do_query(w, w.m_current_query);
			}

			//- Clear queries we have processed so that we only contain relevant ones
			w.m_queries.clear();
		}

		//------------------------------------------------------------------------------------------------------------------------
		void deserialize_entity_recursively(sworld& w, const sscene::sentity& e, const sscene::sentity* p = nullptr)
		{
			std::string_view parent_name_or_uuid;
			if (p)
			{
				//- Assign either the name or uuid, whatever is available
				parent_name_or_uuid = p->m_name.empty() ? p->m_uuid.empty() ? std::string_view{} : p->m_uuid : p->m_name;
			}

			auto entity = entity::create(w, e.m_name, e.m_uuid, parent_name_or_uuid);

			for (const auto& c : e.m_comps)
			{
				if (const auto type = rttr::type::get_by_name(c.m_type_name); type.is_valid())
				{
					rttr::detail::invoke_static_function(type, ecs::detail::C_COMPONENT_SET_FUNC_NAME.data(),
						entity, c.m_data);
				}
			}

			//- After we have deserialized the components we want to update the proxy with the new data of
			//- world and local transforms
			w.m_proxy_manager.update_proxy(entity); //- TODO: we expect that at this point the proxy is already created and not in the creation queue

			for (const auto& kid : e.m_kids)
			{
				deserialize_entity_recursively(w, kid, &e);
			}
		}

		//- A world is a superset of scene format. It contains worlds configuration, singletons and the scene entities,
		//- where the latter can be deserialized to scene.
		//- The deserialize API is user facing, so we want to make sure that when used directly
		//- we still behave like expected for a create function
		//------------------------------------------------------------------------------------------------------------------------
		sworld* deserialize(const nlohmann::json& json)
		{
			CORE_ZONE;

			const auto& j_cfg = json["config"];
			const auto cfg = core::io::from_json_object(rttr::type::get<sconfig>(), j_cfg).get_value<sconfig>();

			const auto& j_name = json["name"];
			const auto name = j_name.get<std::string>();
			CORE_ASSERT(!name.empty(), "Deserialized world does not have a name!");

			sworld* w = nullptr;

			//- Create the world entry
			{
				if (auto [it, result] = cache.emplace(std::piecewise_construct,
					std::forward_as_tuple(algorithm::hash(name)),
					std::forward_as_tuple(name, cfg)); result)
				{
					w = &it->second;
				}
			}

			if (!w)
			{
				return nullptr;
			}

			//- Apply configuration to world, set threads for use, import modules and components etc.
			{
				w->m_world.set_threads(cfg.m_threads);

				//- Import plugins and their modules, note that these can depend on engine and application
				//- modules, i.e. systems and components
				plugins::import(*w, cfg.m_plugins);
				modules::import(*w, cfg.m_modules);
			}

			//- Load singletons for world
			{
				const auto& j_singletons = json["singletons"];
				for (const auto& j_singleton : j_singletons)
				{
					const auto type_name = j_singleton[core::io::C_OBJECT_TYPE_NAME].get<std::string>();

					rttr::variant var;
					rttr::detail::invoke_static_function(rttr::type::get_by_name(type_name), "deserialize", var, j_singleton);

					if (var.is_valid())
					{
						rttr::detail::invoke_static_function(rttr::type::get_by_name(type_name), "set_singleton", *w, var);
					}
				}
			}

			//- Load scene to create a world from
			{
				auto scene = scene::deserialize(json);
				for (const auto& e : scene.m_entities)
				{
					deserialize_entity_recursively(*w, e, nullptr);
				}
			}
			return w;
		}

		//------------------------------------------------------------------------------------------------------------------------
		nlohmann::json serialize(const sworld& w)
		{
			CORE_ZONE;

			nlohmann::json j;

			//- Convert the world to a scene and write to JSON
			{
				const auto scene = detail::world_to_scene(w);
				j["entities"] = nlohmann::json::object();
				j["entities"] = scene::serialize(scene);
			}

			{
				//- Continue serializing worlds singletons, modules and plugins
				j["singletons"] = nlohmann::json::array();

				auto i = 0;
				for (const auto& ct : w.m_active_singletons)
				{
					auto type = rttr::type::get_by_name(ct);

					log_trace(fmt::format("\tbegin to serialize singleton of type '{}'", ct));

					rttr::detail::invoke_static_function(type, "serialize", rttr::detail::invoke_static_function(type, "get_singleton", w), j["singletons"][i++]);
				}

				j["config"] = core::io::to_json_object(w.config());
				j["name"] = w.name();
			}

			return j;
		}

		//------------------------------------------------------------------------------------------------------------------------
		std::string filepath(int id)
		{
			if (const auto it = paths.find(id); it != paths.end())
			{
				return it->second.data();
			}
			return {};
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld* create(std::string_view name_or_filepath, std::optional<sconfig> cfg /*= std::nullopt*/)
		{
			CORE_ZONE;

			const auto h = algorithm::hash(name_or_filepath);

			if (const auto it = cache.find(h); it != cache.end())
			{
				return &it->second;
			}

			auto& vfs = cengine::instance().service<fs::cvirtual_filesystem>();
			if (const auto world_file_scope = vfs.open_file(name_or_filepath, core::file_mode_read); world_file_scope)
			{
				const auto file = world_file_scope.file();

				if (auto future = file->read_async(); future.valid())
				{
					{
						core::casync_wait_scope world_wait_scope(future);
					}

					auto mem = future.get();

					if (mem && !mem->empty())
					{
						auto j = nlohmann::json::parse(mem->data(), mem->data() + mem->size(), nullptr, false, true);

						if (auto* w = deserialize(j); w)
						{
							//- TODO: consider whether to store by the filepath provided or by the worlds name
							paths[w->id()] = { w->name() };
							return w;
						}
						return nullptr;
					}
				}
			}
			else
			{
				if (auto [it, result] = cache.emplace(std::piecewise_construct,
					std::forward_as_tuple(h),
					std::forward_as_tuple(name_or_filepath)); result)
				{
					paths[h] = { name_or_filepath };
					return &it->second;
				}
			}
			return nullptr;
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld* active()
		{
			if (current_active_world == MAXIMUM(uint64_t) ||
				cache.find(current_active_world) == cache.end())
			{
				return nullptr;
			}

			core::cscope_mutex m(mutex);
			return &cache.at(current_active_world);
		}

		//------------------------------------------------------------------------------------------------------------------------
		void promote(sworld& w)
		{
			//- If we have another active world at the moment, we can demote it
			if (current_active_world != MAXIMUM(uint64_t))
			{
				const auto it = cache.find(current_active_world);
				CORE_ASSERT(it != cache.end(), "World to be set to background mode was deleted");

				demote(it->second);
			}

			//- Remove background mode and set foreground mode for given world and update its flags, and set it as active
			const auto& cfg = w.config();
			auto flags = cfg.m_flags;
			algorithm::bit_clear(flags, world_flag_background);
			algorithm::bit_set(flags, world_flag_foreground);

			core::cscope_mutex m(mutex);
			w.flags(flags);
			current_active_world = w.id();
		}

		//------------------------------------------------------------------------------------------------------------------------
		void demote(sworld& w)
		{
			//- Remove foreground mode and set background mode for given world and update its flags
			const auto& cfg = w.config();
			auto flags = cfg.m_flags;
			algorithm::bit_clear(flags, world_flag_foreground);
			algorithm::bit_set(flags, world_flag_background);

			core::cscope_mutex m(mutex);
			w.flags(flags);
			if (current_active_world == w.id())
			{
				current_active_world = MAXIMUM(uint64_t);
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		void destroy(std::string_view name_or_filepath)
		{
			const auto h = algorithm::hash(name_or_filepath);

			if (const auto it = cache.find(h); it != cache.end())
			{
				core::cscope_mutex m(mutex);
				cache.erase(it);
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		void cleanup()
		{
			core::cscope_mutex m(mutex);
			cache.clear();
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld* find(std::string_view name_or_filepath)
		{
			const auto h = algorithm::hash(name_or_filepath);

			if (const auto it = cache.find(h); it != cache.end())
			{
				return &it->second;
			}
			return nullptr;
		}

		//------------------------------------------------------------------------------------------------------------------------
		sscene as_scene(const sworld& w)
		{
			return detail::world_to_scene(w);
		}

		//------------------------------------------------------------------------------------------------------------------------
		aabb_t visible_area(const sworld& w, float width_scale /*= render::C_WORLD_VISIBLE_AREA_SCALE_X*/,
			float height_scale /*= render::C_WORLD_VISIBLE_AREA_SCALE_Y*/)
		{
			aabb_t output;

			if (auto e = query::one<const render::component::srendercamera>(w, [](const render::component::srendercamera& c)
				{
					//- TODO: currently we assume there is only one camera at a time in the world
					return true;
				}); e.is_valid())
			{
				const auto& c = *e.get<render::component::srendercamera>();
				//- Compute the AABB we are currently seeing based on the camera position and the screen width and height
				const float screen_width = (float)raylib::GetRenderWidth();
				const float screen_height = (float)raylib::GetRenderHeight();
				const float half_world_width = (screen_width * 0.5f / c.m_camera.m_zoom) * width_scale;
				const float half_world_height = (screen_height * 0.5f / c.m_camera.m_zoom) * height_scale;
				const float cx = c.m_camera.m_world_target.x;
				const float cy = c.m_camera.m_world_target.y;
				output.m_aabb.lowerBound.x = cx - half_world_width;
				output.m_aabb.upperBound.x = cx + half_world_width;
				output.m_aabb.lowerBound.y = cy - half_world_height;
				output.m_aabb.upperBound.y = cy + half_world_height;
			}
			return output;
		}

	} //- world

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro::world;
	using namespace kokoro::world::components;
	
	rttr::detail::default_constructor<std::vector<core::cuuid>>();

	//------------------------------------------------------------------------------------------------------------------------
	REGISTER_TAG(sinvisible);
	REGISTER_TAG(ssystem);
	REGISTER_TAG(sentity);

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<sidentifier>("sidentifier")
	.prop("m_name", &sidentifier::m_name)
	.prop("m_uuid", &sidentifier::m_uuid)
	;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<shierarchy>("shierarchy")
	.prop("m_parent", &shierarchy::m_parent)
	.prop("m_children", &shierarchy::m_children)
	;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<sprefab>("sprefab")
	.prop("m_filepath", &sprefab::m_filepath)
	;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<sconfig>("sconfig")
		.prop("m_plugins", &sconfig::m_plugins)
		.prop("m_modules", &sconfig::m_modules)
		.prop("m_threads", &sconfig::m_threads)
		.prop("m_flags", &sconfig::m_flags);
}
