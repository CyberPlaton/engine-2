#include <engine/world/world.hpp>
#include <engine/components/common.hpp>
#include <engine/components/camera.hpp>
#include <engine/components/postprocess_volume.hpp>
#include <engine/components/sprite.hpp>
#include <engine/components/transforms.hpp>
#include <engine/components/viewport.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/resource_manager_service.hpp>
#include <engine/services/log_service.hpp>
#include <engine.hpp>
#include <core/io.hpp>
#include <core/registrator.hpp>
#include <fmt.h>

namespace kokoro
{
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
					if (const auto [result, _] = csystem_manager::builtin_phase(rb); result)
					{
						continue;
					}

					auto& modules = systems_module_map.at(rb);

					m.insert(modules.begin(), modules.end());
				}

				for (auto& ra : cfg.m_run_after)
				{
					if (const auto [result, _] = csystem_manager::builtin_phase(ra); result)
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
		template<typename... ARGS>
		rttr::variant invoke_static_function(rttr::type class_type, rttr::string_view function_name, ARGS&&... args)
		{
			if (const auto m = class_type.get_method(function_name); m.is_valid())
			{
				return m.invoke({}, args...);
			}
			return {};
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	std::optional<cworld> cworld::load(const rttr::variant& snapshot)
	{
		auto& log = instance().service<clog_service>();

		if (!snapshot.is_valid())
		{
			log.err("cworld::load - received invalid snapshot variant");
			return std::nullopt;
		}

		const auto& snaps = snapshot.get_value<sworld_snapshot>();
		auto& vfs = instance().service<cvirtual_filesystem_service>();

		filepath_t world_filepath = snaps.m_filepath;

		if (!vfs.exists(world_filepath))
		{
			if (const auto [result, p] = vfs.resolve(world_filepath); result)
			{
				world_filepath = p;
			}
			else
			{
				log.err(fmt::format("cworld::load - could not find fx file '{}'",
					snaps.m_filepath).c_str());
				return std::nullopt;
			}
		}

		if (auto file = vfs.open(world_filepath, file_options_read | file_options_text); file.opened())
		{
			auto mem = file.read_sync();
			if (mem && !mem->empty())
			{
				auto j = nlohmann::json::parse(mem->data(), mem->data() + mem->size(), nullptr, false, true);

				//- Before creating the world get minimal required data out of there
				auto name = j["name"].get<std::string>();
				auto cfg = core::from_json_object(rttr::type::get<cworld::sconfig>(), j["config"]).get_value<cworld::sconfig>();

				cworld world(name, cfg);
				world.deserialize(j);

				return std::move(world);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cworld::unload(cworld& world)
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	cworld::cworld(std::string_view name) :
		cworld(name, sconfig{})
	{
	}

	//------------------------------------------------------------------------------------------------------------------------
	cworld::cworld(std::string_view name, sconfig cfg) :
		m_cfg(cfg),
		m_name(name.data())
	{
		m_world.set_ctx(this);
		m_world.component<world::component::sidentifier>();
		m_world.component<world::component::shierarchy>();
		m_world.component<world::component::sprefab>();
		m_world.component<world::tag::sentity>();
		m_world.component<world::tag::sinvisible>();
		m_world.component<world::component::scamera>();
		m_world.component<world::component::sviewport>();
		m_world.component<world::component::ssprite>();
		m_world.component<world::component::slocal_transform>();
		m_world.component<world::component::sworld_transform>();
		m_world.component<world::component::spostprocess_volume>();

		m_singleton_manager.world(&m_world);
		m_system_manager.world(&m_world);
		m_query_manager.world(&m_world);
		m_entity_manager.world(&m_world);
		m_singleton_manager.init();
		m_system_manager.init();
		m_query_manager.init();
		m_entity_manager.init();
		m_transform_tracker = query_manager().change_tracker<world::component::slocal_transform>();
	}

	//------------------------------------------------------------------------------------------------------------------------
	cworld::~cworld()
	{
		m_transform_tracker.reset();
		m_singleton_manager.shutdown();
		m_system_manager.shutdown();
		m_query_manager.shutdown();
		m_entity_manager.shutdown();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cworld::import_modules(std::vector<std::string> modules)
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
					instance().service<clog_service>().err("Failed to import an unreflected module of type '{}'!", m);
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

				auto var = type.create({ this });
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	sscene cworld::as_scene()
	{
		sscene scene;
		scene.m_name = m_name;

		//- Find entry-point entities that dont have parents and only children
		std::vector<flecs::entity> entries;
		for (const auto& e : entity_manager().all())
		{
			const auto* hier = e.get<world::component::shierarchy>();
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
			world_to_scene_entity_rec(e, scene.m_entities);
		}

		return scene;
	}

	//------------------------------------------------------------------------------------------------------------------------
	math::aabb_t cworld::visible_area(float width_scale /*= 1.0f*/, float height_scale /*= 1.0f*/)
	{
		//- Work in progress
		math::aabb_t output;
		return output;
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cworld::visible_entities(const math::aabb_t& area) -> entities_t
	{
		if (const auto result = query_manager().query_immediate(squery::type_all, squery::intersection_aabb, area); result.is_valid())
		{
			return result.get_value<std::vector<flecs::entity>>();
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cworld::tick(float dt)
	{
		//- Because managers might need to complete work before we tick the world for a new update we split up in pre and post
		query_manager().pre_tick();

		m_world.progress(dt);

		if (auto t = m_transform_tracker.lock(); t) { t->tick(); }

		query_manager().post_tick();
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cworld::deserialize(const nlohmann::json& json)
	{
		//- Note, the name and configuration were already deserialized seeing as the world did get constructed

		//- Apply configuration
		{
			m_world.set_threads(static_cast<int>(m_cfg.m_threads));
			import_modules(m_cfg.m_modules);
		}

		//- Load singletons
		{
			const auto& j_singletons = json["singletons"];
			for (const auto& j_singleton : j_singletons)
			{
				const auto type_name = j_singleton[core::C_OBJECT_TYPE_NAME].get<std::string>();

				rttr::variant var;
				invoke_static_function(rttr::type::get_by_name(type_name), "deserialize", var, j_singleton);

				if (var.is_valid())
				{
					invoke_static_function(rttr::type::get_by_name(type_name), "set_singleton", *this, var);
				}
			}
		}

		//- Load scene and create the world entities
		{
			auto s = scene::deserialize(json);
			for (const auto& e : s.m_entities)
			{
				scene_to_world_entity_rec(e, nullptr);
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cworld::serialize() -> nlohmann::json
	{
		nlohmann::json j;

		//- Convert the world to a scene and write to JSON
		{
			const auto scene = as_scene();
			j["entities"] = nlohmann::json::object();
			j["entities"] = scene::serialize(scene);
		}

		{
			//- Continue serializing worlds singletons, modules and plugins
			j["singletons"] = nlohmann::json::array();

			auto i = 0;
			for (const auto& ct : singleton_manager().all())
			{
				auto type = rttr::type::get_by_name(ct);

				instance().service<clog_service>().trace("\tbegin to serialize singleton of type '{}'",
					ct);

				invoke_static_function(type, "serialize", invoke_static_function(type, "get_singleton", *this), j["singletons"][i++]);
			}

			j["config"] = core::to_json_object(m_cfg);
			j["name"] = m_name;
		}

		return j;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cworld::world_to_scene_entity_rec(flecs::entity e, sscene::entities_t& entities)
	{
		const auto* id = e.get<world::component::sidentifier>();

		//- Serialize entity basic information
		auto& se = entities.emplace_back();
		se.m_name = id->m_name;
		se.m_uuid = id->m_uuid.string();

		//- Serialize entity components
		for (const auto& c : entity_manager().comps(e))
		{
			auto& sc = se.m_comps.emplace_back();

			auto type = rttr::type::get_by_name(c);

			sc.m_data = invoke_static_function(type, "get", e);
			sc.m_type_name = type.get_name().data();
		}

		//- Serialize recursively children of the entity
		const auto* hier = e.get<world::component::shierarchy>();

		for (const auto& uuid : hier->m_children)
		{
			if (auto kid = entity_manager().find(uuid.string()); kid.is_valid())
			{
				world_to_scene_entity_rec(kid, se.m_kids);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cworld::scene_to_world_entity_rec(const sscene::sentity& e, const sscene::sentity* p /*= nullptr*/)
	{
		std::string_view parent_name_or_uuid;
		if (p)
		{
			//- Assign either the name or uuid, whatever is available
			parent_name_or_uuid = p->m_name.empty() ? p->m_uuid.empty() ? std::string_view{} : p->m_uuid : p->m_name;
		}

		auto entity = entity_manager().create(e.m_name, e.m_uuid, parent_name_or_uuid);

		for (const auto& c : e.m_comps)
		{
			if (const auto type = rttr::type::get_by_name(c.m_type_name); type.is_valid())
			{
				invoke_static_function(type, ecs::C_COMPONENT_SET_FUNC_NAME.data(),
					entity, c.m_data);
			}
		}

		//- After we have deserialized the components we want to update the proxy with the new data of
		//- world and local transforms
		query_manager().update_proxy(entity); //- TODO: we expect that at this point the proxy is already created and not in the creation queue

		for (const auto& kid : e.m_kids)
		{
			scene_to_world_entity_rec(kid, &e);
		}
	}

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<cworld::sconfig>("cworld::sconfig")
		.prop("m_plugins", &cworld::sconfig::m_plugins)
		.prop("m_modules", &cworld::sconfig::m_modules)
		.prop("m_threads", &cworld::sconfig::m_threads)
		.prop("m_flags", &cworld::sconfig::m_flags);
}