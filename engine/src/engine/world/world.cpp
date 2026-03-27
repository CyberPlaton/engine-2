#include <engine/world/world.hpp>
#include <engine/world/prefab.hpp>
#include <engine/services/thread_service.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/components/sprite.hpp>
#include <engine/components/camera.hpp>
#include <engine/components/transforms.hpp>
#include <engine/components/viewport.hpp>
#include <core/hash.hpp>
#include <core/mutex.hpp>
#include <core/uuid.hpp>
#include <core/registrator.hpp>
#include <array>
#include <unordered_set>
#include <unordered_map>

namespace kokoro
{
	namespace world
	{
		namespace detail
		{
			//------------------------------------------------------------------------------------------------------------------------
			template<typename... ARGS>
			rttr::variant invoke_static_function(rttr::type class_type, rttr::string_view function_name, ARGS&&... args)
			{
				if (const auto m = class_type.get_method(function_name); m.is_valid())
				{
					return m.invoke({}, args...);
				}
				return {};
			}

			//------------------------------------------------------------------------------------------------------------------------
			std::pair<bool, uint64_t> do_check_is_flecs_built_in_phase(std::string_view name)
			{
				static std::array<std::pair<std::string, uint64_t>, 8> C_PHASES =
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

				if (const auto& it = std::find_if(C_PHASES.begin(), C_PHASES.end(), [=](const auto& pair)
					{
						return pair.first == name;

					}); it != C_PHASES.end())
				{
					return { true, (uint64_t)it->second };
				}

				return { false, std::numeric_limits<uint64_t>().max() };
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

					sc.m_data = invoke_static_function(type, "get", e);
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
				std::vector<std::string> m_run_before;
				std::vector<std::string> m_run_after;
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

			constexpr auto C_MAXIMUM = std::numeric_limits<uint64_t>().max();
			core::cmutex mutex;
			std::unordered_map<uint64_t, sworld> cache;
			std::unordered_map<int, filepath_t> paths;
			uint64_t current_active_world = std::numeric_limits<uint64_t>().max();

			//------------------------------------------------------------------------------------------------------------------------
			kokoro::modules::sconfig get_module_config(std::string_view type)
			{
				if (const auto t = rttr::type::get_by_name(type.data()); t.is_valid())
				{
					if (const auto m = t.get_method("config"); m.is_valid())
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
					if (const auto m = t.get_method("config"); m.is_valid())
					{
						return m.invoke({}).convert<kokoro::system::sconfig>();
					}
				}
				return {};
			}

			//- Function takes in raw modules to be imported and creates a sorted order of import. We order by explicit module to module
			//- dependency and implicit module-system to module dependencies.
			//------------------------------------------------------------------------------------------------------------------------
			std::vector<std::string> topsort(const std::vector<simported_module>& data, bool verbose)
			{
				std::unordered_map<std::string, std::unordered_set<std::string>> systems_module_map; //- Mapping all systems to their dependency modules
				std::unordered_map<std::string, std::unordered_set<std::string>> modules_to_module_map; //- Mapping all modules to their dependency modules

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

				const auto do_topsort = [](const std::unordered_map<std::string, std::unordered_set<std::string>>& graph) -> std::vector<std::string>
					{
						std::vector<std::string> sorted;
						std::unordered_set<std::string> visited;
						std::unordered_set<std::string> recursion_stack;

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
									//- Cyclic module dependency
									return {};
								}
							}
						}

						return sorted;
					};

				auto result = do_topsort(modules_to_module_map);
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
				case squery::type_none:		return {};
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			void do_query(sworld& w, const squery& q)
			{
				w.m_query_results[w.m_current_query_key] = result_type(q.m_type);
				auto& tree = w.m_proxy_manager.m_quad_tree;

				if (w.m_current_query.m_intersection == squery::intersection_aabb &&
					q.m_data.get_type() == rttr::type::get<math::aabb_t>())
				{
					box2d::b2DynamicTree_Query(&tree, q.m_data.get_value<math::aabb_t>(), B2_DEFAULT_MASK_BITS, &query::detail::query_callback, &w);
				}

				else if (w.m_current_query.m_intersection == squery::intersection_ray &&
					q.m_data.get_type() == rttr::type::get<math::ray_t>())
				{
					box2d::b2RayCastInput ray = q.m_data.get_value<math::ray_t>();

					box2d::b2DynamicTree_RayCast(&tree, &ray, B2_DEFAULT_MASK_BITS, &query::detail::raycast_callback, &w);
				}
			}

		} //- unnamed

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::sproxy_manager::tick()
		{
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
			if (const auto* id = e.get<components::sidentifier>(); id)
			{
				if (const auto it = m_proxies.find(id->m_uuid); it != m_proxies.end())
				{
					auto& proxy = m_proxies[id->m_uuid];
					const auto* transform = e.get<component::sworld_transform>();
					proxy.m_aabb = transform->m_aabb;
					box2d::b2DynamicTree_MoveProxy(&m_quad_tree, proxy.m_id, transform->m_aabb);
				}
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::sproxy_manager::destroy_proxy(flecs::entity e)
		{
			if (const auto* id = e.get<components::sidentifier>(); id)
			{
				if (const auto it = m_proxies.find(id->m_uuid); it != m_proxies.end())
				{
					box2d::b2DynamicTree_DestroyProxy(&m_quad_tree, it->second.m_id);
					m_proxies.erase(it);
				}
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::sproxy_manager::create_proxy(flecs::entity e)
		{
			const auto* id = e.get<components::sidentifier>();
			const auto* transform = e.get<component::sworld_transform>();
			const auto data_id = user_data_id(e);
			auto& proxy = m_proxies[id->m_uuid];
			proxy.m_aabb = transform->m_aabb;
			proxy.m_entity = e;
			proxy.m_id = box2d::b2DynamicTree_CreateProxy(&m_quad_tree, transform->m_aabb, B2_DEFAULT_CATEGORY_BITS, data_id);

			m_user_data[data_id] = id->m_uuid;
		}

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::sproxy_manager::queue_create_proxy(flecs::entity e)
		{
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

			static sproxy S_DUMMY;
			return S_DUMMY;
		}

		//------------------------------------------------------------------------------------------------------------------------
		int sworld::sproxy_manager::user_data_id(const flecs::entity& e) const
		{
			return core::hash(e.get<components::sidentifier>()->m_uuid.string());
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld::sworld(std::string_view name) :
			sworld(name, {})
		{
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld::sworld(std::string_view name, sconfig cfg)
			: m_name(name.data()), m_id(core::hash(m_name)), m_cfg(cfg)
		{
			m_proxy_manager.m_quad_tree = box2d::b2DynamicTree_Create();

			auto& w = m_world;
			w.set_ctx(this);
			w.component<components::sidentifier>();
			w.component<components::shierarchy>();
			w.component<components::sprefab>();
			w.component<components::tag::sentity>();
			w.component<components::tag::sinvisible>();
			w.component<components::tag::ssystem>();
			w.component<component::scamera>();
			w.component<component::sviewport>();
			w.component<component::ssprite>();
			w.component<component::slocal_transform>();
			w.component<component::sworld_transform>();

			m_proxy_observer = w
				.observer<component::ssprite, component::slocal_transform, components::sidentifier>()
				.event(flecs::OnAdd)
				.event(flecs::OnRemove)
				.ctx(this)
				.each([](flecs::iter& it, uint64_t i,
					component::ssprite&,
					component::slocal_transform&,
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
				.each([](flecs::iter& it, uint64_t i)
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
								const auto is_tag = string.find(".tag") != std::string::npos;
								auto c = string.substr(last + 1);
								if (is_tag) { c = fmt::format("tag::{}", c); }

								if (it.event() == flecs::OnAdd)
								{
									instance().service<clog_service>().debug("Component added '{}({})' to entity '{}'",
										c, string, entity.id());

									//- name of the component will be in form 'ecs.transform', we have to format the name first
									world->m_entity_comps[entity].insert(c);
								}
								else if (it.event() == flecs::OnRemove)
								{
									instance().service<clog_service>().debug("Component removed '{}({})' from entity '{}'",
										c, string, entity.id());

									world->m_entity_comps[entity].erase(c);
								}
							}
						}
					});

			m_transform_tracker = query::change_tracker<component::slocal_transform>(*this);
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld::~sworld()
		{
			box2d::b2DynamicTree_Destroy(&m_proxy_manager.m_quad_tree);
			m_proxy_observer.destruct();
			m_comps_observer.destruct();
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool sworld::foreground() const
		{
			return !!(m_cfg.m_flags & world_flag_foreground) && id() == current_active_world;
		}

		//------------------------------------------------------------------------------------------------------------------------
		void sworld::flags(world_flags_t value)
		{
			m_cfg.m_flags = value;
		}

		namespace components
		{
			//------------------------------------------------------------------------------------------------------------------------
			bool sidentifier::show_ui(rttr::variant& comp)
			{
				return false;
			}

			//------------------------------------------------------------------------------------------------------------------------
			bool shierarchy::show_ui(rttr::variant& comp)
			{
				return false;
			}

			//------------------------------------------------------------------------------------------------------------------------
			bool sprefab::show_ui(rttr::variant& comp)
			{
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

		namespace modules
		{
			//------------------------------------------------------------------------------------------------------------------------
			void import(sworld& w, std::vector<std::string> modules)
			{
				std::vector<simported_module> unsorted_modules;
				std::vector<std::string> sorted_modules;

				//- Transform raw data to a form that we can sort and reason about
				{
					for (const auto& m : modules)
					{
						auto config = get_module_config(m);

						if (config.m_name.empty())
						{
							instance().service<clog_service>().err("Failed to import an unreflected module of type '{}'!",
								m);
							return;
						}

						//- Do not process module if already imported
						for (const auto& m : unsorted_modules)
						{
							if (m.m_name == config.m_name)
							{
								return;
							}
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
				auto builder = w.m_world.system(cfg.m_name.c_str());

				//- Set options that are required before system entity creation
				{
					if (!!(cfg.m_flags & system::system_flag_multithreaded))
					{
						builder.multi_threaded();
					}
					else if (!!(cfg.m_flags & system::system_flag_immediate))
					{
						builder.immediate();
					}
				}

				if (const auto duplicate_system = find_system(w, cfg.m_name); duplicate_system)
				{
					instance().service<clog_service>().err("Trying to create a system with same name twice '{}'. This is not allowed!",
						cfg.m_name);
					return;
				}

				if (cfg.m_interval != 0)
				{
					builder.interval(static_cast<float>(cfg.m_interval));
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
								instance().service<clog_service>().err("Dependency (run after) system '{}' for system '{}' could not be found!",
									after, cfg.m_name);
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
								instance().service<clog_service>().err("Dependent (run before) system '{}' for system '{}' could not be found!",
									before, cfg.m_name);
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
				detail::invoke_static_function(c.get_type(), kokoro::ecs::C_COMPONENT_ADD_SINGLETON_FUNC_NAME, w.m_world);
				w.m_active_singletons.insert(c.get_type().get_name().data());

			}

			//------------------------------------------------------------------------------------------------------------------------
			void remove(sworld& w, const rttr::type& t)
			{
				detail::invoke_static_function(t, kokoro::ecs::C_COMPONENT_REMOVE_SINGLETON_FUNC_NAME, w.m_world);
				w.m_active_singletons.erase(t.get_name().data());
			}

		} //- singletons

		namespace entity
		{
			//------------------------------------------------------------------------------------------------------------------------
			void add(sworld& w, flecs::entity e, const rttr::variant& c)
			{
				detail::invoke_static_function(c.get_type(), kokoro::ecs::C_COMPONENT_SET_FUNC_NAME, e, c);
				w.m_entity_comps[e.id()].insert(c.get_type().get_name().data());
			}

			//------------------------------------------------------------------------------------------------------------------------
			void remove(sworld& w, flecs::entity e, const rttr::type& t)
			{
				detail::invoke_static_function(t, kokoro::ecs::C_COMPONENT_REMOVE_FUNC_NAME, e);

				std::string_view type_name = t.get_name().data();
				auto& comps = w.m_entity_comps[e.id()];

				for (const auto& ct : comps)
				{
					if (ct == type_name)
					{
						comps.erase(ct);
						return;
					}
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			flecs::entity create(sworld& w, std::string_view name, std::string_view uuid /*= {}*/,
				std::string_view parent /*= {}*/, bool is_prefab /*= false*/)
			{
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
				add<component::sworld_transform>(w, out);
				add<component::slocal_transform>(w, out);

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
						const auto s = id->m_uuid;
						const auto h = s.hash();

						for (auto i = 0; i < hierarchy->m_children.size(); ++i)
						{
							const auto& k = hierarchy->m_children[i];

							if (k.hash() == h)
							{
								hierarchy->m_children.erase(hierarchy->m_children.begin() + i);
								break;
							}
						}
					}
				}

				//- Destroy entity within ecs world to free components and data
				e.destruct();

				//- Untrack entity and destroy its proxy
				for (auto i = 0; i < w.m_all_entities.size(); ++i)
				{
					const auto& entity = w.m_all_entities[i];

					if (entity == e)
					{
						w.m_all_entities.erase(w.m_all_entities.begin() + i);
						break;
					}
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
				w.m_query_results.erase(it);
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
					w.m_query_results.erase(it);
				}
				return out;
			}

			namespace detail
			{
				//------------------------------------------------------------------------------------------------------------------------
				bool query_callback(int proxy_id, int user_data, void* ctx)
				{
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
		std::vector<flecs::entity> visible_entities(const sworld& w, const math::aabb_t& area)
		{
			if (const auto result = query::immediate(const_cast<sworld&>(w),
				squery::type_all,
				squery::intersection_aabb,
				area); result.is_valid())
			{
				return result.get_value<std::vector<flecs::entity>>();
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
					detail::invoke_static_function(type, ecs::C_COMPONENT_SET_FUNC_NAME.data(),
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
			const auto& j_cfg = json["config"];
			const auto cfg = core::from_json_object(rttr::type::get<sconfig>(), j_cfg).get_value<sconfig>();

			const auto& j_name = json["name"];
			const auto name = j_name.get<std::string>();

			sworld* w = nullptr;

			//- Create the world entry
			{
				if (auto [it, result] = cache.emplace(std::piecewise_construct,
					std::forward_as_tuple(core::hash(name)),
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
				modules::import(*w, cfg.m_modules);
			}

			//- Load singletons for world
			{
				const auto& j_singletons = json["singletons"];
				for (const auto& j_singleton : j_singletons)
				{
					const auto type_name = j_singleton[core::C_OBJECT_TYPE_NAME].get<std::string>();

					rttr::variant var;
					detail::invoke_static_function(rttr::type::get_by_name(type_name), "deserialize", var, j_singleton);

					if (var.is_valid())
					{
						detail::invoke_static_function(rttr::type::get_by_name(type_name), "set_singleton", *w, var);
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

					instance().service<clog_service>().trace("\tbegin to serialize singleton of type '{}'",
						ct);

					detail::invoke_static_function(type, "serialize", detail::invoke_static_function(type, "get_singleton", w), j["singletons"][i++]);
				}

				j["config"] = core::to_json_object(w.config());
				j["name"] = w.name();
			}

			return j;
		}

		//------------------------------------------------------------------------------------------------------------------------
		std::string filepath(int id)
		{
			if (const auto it = paths.find(id); it != paths.end())
			{
				return it->second.generic_string();
			}
			return {};
		}

		//- Note: we do not resolve the filepath using VFS  on purpose, this is the responsibility of the caller
		//------------------------------------------------------------------------------------------------------------------------
		sworld* create(std::string_view name_or_filepath, std::optional<sconfig> cfg /*= std::nullopt*/)
		{
			const auto h = core::hash(name_or_filepath);

			if (const auto it = cache.find(h); it != cache.end())
			{
				return &it->second;
			}

			auto& vfs = instance().service<cvirtual_filesystem_service>();

			if (auto wrapper = vfs.open(name_or_filepath, file_options_read | file_options_text); wrapper)
			{
				auto& file = wrapper.get();
				auto mem = file.read_sync();

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
			if (current_active_world == C_MAXIMUM ||
				cache.find(current_active_world) == cache.end())
			{
				return nullptr;
			}

			core::cscoped_mutex m(mutex);
			return &cache.at(current_active_world);
		}

		//------------------------------------------------------------------------------------------------------------------------
		void promote(sworld& w)
		{
			//- If we have another active world at the moment, we can demote it
			if (current_active_world != C_MAXIMUM)
			{
				const auto it = cache.find(current_active_world);
				demote(it->second);
			}

			//- Remove background mode and set foreground mode for given world and update its flags, and set it as active
			const auto& cfg = w.config();
			auto flags = cfg.m_flags;

			flags &= ~world_flag_background;
			flags |= world_flag_foreground;

			core::cscoped_mutex m(mutex);
			w.flags(flags);
			current_active_world = w.id();
		}

		//------------------------------------------------------------------------------------------------------------------------
		void demote(sworld& w)
		{
			//- Remove foreground mode and set background mode for given world and update its flags
			const auto& cfg = w.config();
			auto flags = cfg.m_flags;

			flags &= ~world_flag_foreground;
			flags |= world_flag_background;

			core::cscoped_mutex m(mutex);
			w.flags(flags);
			if (current_active_world == w.id())
			{
				current_active_world = C_MAXIMUM;
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		void destroy(std::string_view name_or_filepath)
		{
			const auto h = core::hash(name_or_filepath);

			if (const auto it = cache.find(h); it != cache.end())
			{
				core::cscoped_mutex m(mutex);
				cache.erase(it);
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		void cleanup()
		{
			core::cscoped_mutex m(mutex);
			cache.clear();
		}

		//------------------------------------------------------------------------------------------------------------------------
		sworld* find(std::string_view name_or_filepath)
		{
			const auto h = core::hash(name_or_filepath);

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
		math::aabb_t visible_area(const sworld& w, float width_scale /*= render::C_WORLD_VISIBLE_AREA_SCALE_X*/,
			float height_scale /*= render::C_WORLD_VISIBLE_AREA_SCALE_Y*/)
		{
			math::aabb_t output;
			return output;
		}

	} //- world

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro::world;
	using namespace kokoro::world::components;
	
	rttr::detail::default_constructor<std::vector<kokoro::core::cuuid>>();

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
