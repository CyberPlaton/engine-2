#include <engine/world/prefab.hpp>
#include <core/hash.hpp>
#include <core/mutex.hpp>
#include <engine/world/world.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/command_service.hpp>
#include <engine/services/log_service.hpp>
#include <engine/commands/update_component_command.hpp>
#include <engine.hpp>
#include <fmt.h>

namespace kokoro
{
	namespace world
	{
		namespace
		{
			core::cmutex mutex;
			std::unordered_map<std::string, flecs::entity> cache;
			std::unordered_map<std::string, filepath_t> paths;

			//------------------------------------------------------------------------------------------------------------------------
			void create_entity_recursive(sworld& w, const sscene::sentity& e, const core::cuuid& parent_uuid)
			{
				auto flecs_entity = entity::create(w, e.m_name, e.m_uuid, parent_uuid.string());
				auto uuid = flecs_entity.get<components::sidentifier>()->m_uuid;

				//- Set components as they are defined in prefab
				for (const auto& c : e.m_comps)
				{
					if (const auto type = rttr::type::get_by_name(c.m_type_name); type.is_valid())
					{
						if (const auto method = type.get_method(ecs::C_COMPONENT_SET_FUNC_NAME.data()); method.is_valid())
						{
							method.invoke({}, flecs_entity, c.m_data);
						}
						else
						{
							//- RTTR error, component does not have a set function
							instance().service<clog_service>().err(fmt::format("\tcould not find 'set' method for component '{}'", c.m_type_name).c_str());
						}
					}
					else
					{
						//- Type name provided is invalid and not reflected to RTTR
						instance().service<clog_service>().err(fmt::format("\tcomponent '{}' not registered to RTTR", c.m_type_name).c_str());
					}
				}

				//- Recursively create children
				for (const auto& k : e.m_kids)
				{
					create_entity_recursive(w, k, uuid);
				}
			}

		} //- unnamed

		namespace prefab
		{
			//------------------------------------------------------------------------------------------------------------------------
			nlohmann::json serialize(const sscene& prefab)
			{
				return scene::serialize(prefab);
			}

			//------------------------------------------------------------------------------------------------------------------------
			sscene deserialize(const nlohmann::json& json)
			{
				return scene::deserialize(json);
			}

			//------------------------------------------------------------------------------------------------------------------------
			std::string filepath(const core::cuuid& uuid)
			{
				if (const auto it = paths.find(uuid.string()); it != paths.end())
				{
					return it->second.generic_string();
				}
				return {};
			}

			//------------------------------------------------------------------------------------------------------------------------
			core::cuuid create(sworld* w, std::string_view filepath, std::optional<sconfig> cfg /*= std::nullopt*/)
			{
				//- Load prefab file
				auto& vfs = instance().service<cvirtual_filesystem_service>();
				if (const auto file = vfs.open(filepath, file_options_read | file_options_text); file)
				{
					auto future = file->read_async();

					while (future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::ready) {}
					file->close();

					auto mem = future.get();

					if (mem && !mem->empty())
					{
						//- Read JSON and convert to prefab data, then instantiate from that
						if (auto json = nlohmann::json::parse(mem->begin(), mem->end(), nullptr, false); !json.is_discarded() || !json.empty())
						{
							auto prefab = scene::deserialize(json);

							if (const auto uuid = create(w, prefab, cfg); uuid != core::cuuid::C_INVALID_UUID)
							{
								auto& cs = cengine::instance().service<ccommand_system_service>();
								{
									components::sprefab c;
									c.m_filepath = filepath;
									cs.push<command::cupdate_component_command>(w, uuid.string(), std::move(rttr::variant(c)));
								}

								//- Store uuid to filepath for later lookup
								core::cscoped_mutex m(mutex);
								paths[uuid.string()] = { filepath };
								return uuid;
							}
						}
					}
				}

				return core::cuuid::C_INVALID_UUID;
			}

			//------------------------------------------------------------------------------------------------------------------------
			core::cuuid create(sworld* w, const sscene& data, std::optional<sconfig> cfg /*= std::nullopt*/)
			{
				//- Create root entity that is designated as the prefab
				auto root = entity::create(*w, data.m_name, {}, {}, true);
				auto uuid = root.get<components::sidentifier>()->m_uuid;

				for (const auto& e : data.m_entities)
				{
					create_entity_recursive(*w, e, uuid);
				}

				core::cscoped_mutex m(mutex);
				cache[uuid.string()] = root;
				return uuid;
			}

			//------------------------------------------------------------------------------------------------------------------------
			void destroy(sworld* w, const core::cuuid& uuid)
			{
				if (const auto it = cache.find(uuid.string()); it != cache.end())
				{
					entity::kill(*w, it->first);

					core::cscoped_mutex m(mutex);
					cache.erase(it);
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			void cleanup(sworld* w)
			{
				core::cscoped_mutex m(mutex);
				for (const auto& [uuid, _] : cache)
				{
					entity::kill(*w, uuid);
				}
				cache.clear();
			}

		} //- prefab

	} //- world

} //- kokoro
