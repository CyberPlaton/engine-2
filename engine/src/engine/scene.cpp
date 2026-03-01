#include <engine/scene.hpp>
#include <core/io.hpp>
#include <engine/world/component.hpp>
#include <registrator.hpp>
#include <engine/services/log_service.hpp>
#include <engine.hpp>
#include <fmt.h>

namespace kokoro
{
	namespace scene
	{
		namespace
		{
			//------------------------------------------------------------------------------------------------------------------------
			void serialize_entity_recursive(const sscene::sentity& e, nlohmann::json& j)
			{
				j = nlohmann::json::object();
				j["name"] = e.m_name;
				j["uuid"] = e.m_uuid;
				j[C_COMPONENTS_PROP] = nlohmann::json::array();
				j[C_ENTITIES_PROP] = nlohmann::json::array();

				//- Write entities components
				auto i = 0;
				for (const auto& c : e.m_comps)
				{
					const auto type = rttr::type::get_by_name(c.m_type_name);
					const auto idx = i++;

					j[C_COMPONENTS_PROP][idx] = nlohmann::json::object();
					j[C_COMPONENTS_PROP][idx][kokoro::core::C_OBJECT_TYPE_NAME] = type.get_name().data();

					instance().service<clog_service>().trace(fmt::format("\tbegin to serialize component of type '{}'", type.get_name().data()).c_str());

					//- Attempt to serialize component, whether custom serialize or default serialize will be called
					//- depends on the component implementation
					if (const auto method = type.get_method(ecs::C_COMPONENT_SERIALIZE_FUNC_NAME.data()); method.is_valid())
					{
						method.invoke({}, c.m_data, j[C_COMPONENTS_PROP][idx]);
					}
					else
					{
						//- RTTR error, component does not have a serialize function
						instance().service<clog_service>().err(fmt::format("\tcould not find 'serialize' method for component '{}'", c.m_type_name).c_str());
					}
				}

				//- Write entities kids recursively
				auto k = 0;
				for (const auto& kid : e.m_kids)
				{
					serialize_entity_recursive(kid, j[C_ENTITIES_PROP][k++]);
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			nlohmann::json do_serialize(const sscene& scene)
			{
				nlohmann::json j;
				j = nlohmann::json::object();
				j["name"] = scene.m_name;
				j[C_ENTITIES_PROP] = nlohmann::json::array();

				auto i = 0;
				for (const auto& e : scene.m_entities)
				{
					serialize_entity_recursive(e, j[C_ENTITIES_PROP][i++]);
				}

				return j;
			}

			//------------------------------------------------------------------------------------------------------------------------
			void deserialize_entity_recursive(sscene::sentity& e, const nlohmann::json& json)
			{
				e.m_name = json.contains("name") ? json["name"] : "<none>";
				e.m_uuid = json.contains("uuid") ? json["uuid"] : "<none>";

				//- Read components
				if (json.contains(C_COMPONENTS_PROP))
				{
					for (const auto& cit : json[C_COMPONENTS_PROP])
					{
						if (cit.contains(core::C_OBJECT_TYPE_NAME))
						{
							const auto type_name = cit[core::C_OBJECT_TYPE_NAME].get<std::string>();
							if (const auto type = rttr::type::get_by_name(type_name); type.is_valid())
							{
								if (const auto method = type.get_method(ecs::C_COMPONENT_DESERIALIZE_FUNC_NAME.data()); method.is_valid())
								{
									auto& comp = e.m_comps.emplace_back();
									comp.m_type_name = type_name;

									method.invoke({}, comp.m_data, cit);
								}
								else
								{
									//- RTTR error, component does not have a deserialize function
									instance().service<clog_service>().err(fmt::format("\tcould not find 'deserialize' method for component '{}'", type_name).c_str());
								}
							}
							else
							{
								//- Type name provided is invalid and not reflected to RTTR
								instance().service<clog_service>().err(fmt::format("\tcomponent '{}' not registered to RTTR", type_name).c_str());
							}
						}
						else
						{
							//- We do not have type information and cannot deserialize component
							instance().service<clog_service>().err(fmt::format("\tno component type name provided in \n'{}'", cit.dump(4)).c_str());
						}
					}
				}

				//- Read children data recursively
				if (json.contains(C_ENTITIES_PROP))
				{
					for (const auto& it : json[C_ENTITIES_PROP])
					{
						auto& kid = e.m_kids.emplace_back();

						deserialize_entity_recursive(kid, it);
					}
				}
			}

			//------------------------------------------------------------------------------------------------------------------------
			sscene do_deserialize(const nlohmann::json& json)
			{
				sscene scene;
				scene.m_name = json.contains("name") ? json["name"].get<std::string>() : std::string{};

				if (json.contains(C_ENTITIES_PROP))
				{
					for (const auto& it : json[C_ENTITIES_PROP])
					{
						auto& e = scene.m_entities.emplace_back();
						deserialize_entity_recursive(e, it);
					}
				}
				return scene;
			}

		} //- unnamed

		//------------------------------------------------------------------------------------------------------------------------
		nlohmann::json serialize(const sscene& scene)
		{
			return do_serialize(scene);
		}

		//------------------------------------------------------------------------------------------------------------------------
		sscene deserialize(const nlohmann::json& json)
		{
			return do_deserialize(json);
		}

	} //- scene

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro::scene;
	using namespace kokoro;

	rttr::detail::default_constructor<sscene::components_t>();
	rttr::detail::default_constructor<sscene::entities_t>();

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<sscene::scomponent>("sscene::scomponent")
		.prop("m_type_name", &sscene::scomponent::m_type_name)
		.prop("m_data", &sscene::scomponent::m_data)
		;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<sscene::sentity>("sscene::sentity")
		.prop("m_name", &sscene::sentity::m_name)
		.prop("m_uuid", &sscene::sentity::m_uuid)
		.prop("m_comps", &sscene::sentity::m_comps)
		.prop("m_kids", &sscene::sentity::m_kids)
		;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<sscene>("sscene")
		.prop("m_name", &sscene::m_name)
		.prop("m_entities", &sscene::m_entities)
		;
}
