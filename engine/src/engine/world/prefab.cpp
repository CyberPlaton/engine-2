#include <engine/world/prefab.hpp>
#include <core/hash.hpp>
#include <core/mutex.hpp>
#include <engine/components/common.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/command_service.hpp>
#include <engine/services/log_service.hpp>
#include <engine/commands/update_component_command.hpp>
#include <engine/world/world.hpp>
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
			void create_entity_recursive(cview<cworld> w, const sscene::sentity& e, const core::cuuid& parent_uuid)
			{
				if (!w.valid())
				{
					return;
				}

				auto flecs_entity = w.get().entity_manager().create(e.m_name, e.m_uuid, parent_uuid.string());
				auto uuid = flecs_entity.get<component::sidentifier>()->m_uuid;

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

			//- Note: we do not resolve the filepath using VFS  on purpose, this is the responsibility of the caller
			//------------------------------------------------------------------------------------------------------------------------
			core::cuuid create(cview<cworld> w, std::string_view filepath, std::optional<sconfig> cfg /*= std::nullopt*/)
			{
				if (!w.valid())
				{
					return {};
				}

				auto& vfs = instance().service<cvirtual_filesystem_service>();

				if (auto file = vfs.open(filepath, file_options_read | file_options_text); file.opened())
				{
					auto mem = file.read_sync();

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
									component::sprefab c;
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
			core::cuuid create(cview<cworld> w, const sscene& data, std::optional<sconfig> cfg /*= std::nullopt*/)
			{
				if (w.valid())
				{
					//- Create root entity that is designated as the prefab
					auto root = w.get().entity_manager().create(data.m_name, {}, {}, true);
					auto uuid = root.get<component::sidentifier>()->m_uuid;

					for (const auto& e : data.m_entities)
					{
						create_entity_recursive(w, e, uuid);
					}

					core::cscoped_mutex m(mutex);
					cache[uuid.string()] = root;
					return uuid;
				}
				return {};
			}

			//------------------------------------------------------------------------------------------------------------------------
			void destroy(cview<cworld> w, const core::cuuid& uuid)
			{
				core::cscoped_mutex m(mutex);

				if (const auto it = cache.find(uuid.string()); it != cache.end() && w.valid())
				{
					w.get().entity_manager().kill(uuid.string());
					cache.erase(it);
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			void cleanup(cview<cworld> w)
			{
				core::cscoped_mutex m(mutex);

				for (const auto& [uuid, _] : cache)
				{
					w.get().entity_manager().kill(uuid);
				}
				cache.clear();
			}

		} //- prefab

	} //- world

} //- kokoro
