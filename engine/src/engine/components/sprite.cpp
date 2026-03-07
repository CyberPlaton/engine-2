#include <engine/components/sprite.hpp>
#include <registrator.hpp>

namespace kokoro::world::component
{
	//------------------------------------------------------------------------------------------------------------------------
	bool ssprite::show_ui(rttr::variant& comp)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ssprite::custom_serialize(const rttr::variant& var, nlohmann::json& json)
	{
		if (const auto type = var.get_type(); type.is_valid() && type == rttr::type::get<ssprite>())
		{
			const auto& s = var.get_value<ssprite>();
			const auto type_name = type.get_name().data();

			json = nlohmann::json::object();
			json[core::C_OBJECT_TYPE_NAME] = type_name;

			//- Serialize sprite data
			nlohmann::json j;
			j = nlohmann::json::object();

			//- Serialize material
			if (s.m_material)
			{
				auto& mrm = instance().service<cmaterial_resource_manager_service>();
				const auto path = mrm.filepath(s.m_material);
				j["material"] = path.generic_string();
			}
			if (s.m_mesh)
			{
				auto& mrm = instance().service<cmesh_resource_manager_service>();
				const auto path = mrm.filepath(s.m_mesh);
				j["mesh"] = path.generic_string();
			}
			j["source"] = core::to_json_object(s.m_source);
			j["uv0"] = core::to_json_object(s.m_uv0);
			j["uv1"] = core::to_json_object(s.m_uv1);
			j["tint"] = s.m_tint;

			json[type_name] = j;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ssprite::custom_deserialize(rttr::variant& var, nlohmann::json& json)
	{
		if (json.contains(core::C_OBJECT_TYPE_NAME))
		{
			const auto type_name = json[core::C_OBJECT_TYPE_NAME].get<std::string>();
			if (const auto type = rttr::type::get_by_name(type_name); type.is_valid() && type == rttr::type::get<ssprite>())
			{
				auto& material_manager = instance().service<cmaterial_resource_manager_service>();
				auto& mesh_manager = instance().service<cmesh_resource_manager_service>();

				auto& j = json[type_name]; //- TODO: now this does look redundant and we probably should automate this part
				auto& s = var.get_value<ssprite>();

				auto material_path = j["material"].get<std::string>();
				auto mesh_path = j["mesh"].get<std::string>();

				{
					s.m_material = material_manager.instantiate(material_path);
				}

				{
					s.m_mesh = mesh_manager.instantiate(mesh_path);
				}

				s.m_source = core::from_json_object(rttr::type::get<math::vec4_t>(), j["source"]).get_value<math::vec4_t>();
				s.m_tint = j["tint"].get<uint32_t>();

				const auto has_uv0 = j.contains("uv0");
				const auto has_uv1 = j.contains("uv1");

				if (has_uv0)
				{
					s.m_uv0 = core::from_json_object(rttr::type::get<math::vec2_t>(), j["uv0"]).get_value<math::vec2_t>();
				}
				if (has_uv1)
				{
					s.m_uv1 = core::from_json_object(rttr::type::get<math::vec2_t>(), j["uv1"]).get_value<math::vec2_t>();
				}

				//- Reconstruct uvs from source rectangle if any of them is missing
				if (!(has_uv0 && has_uv1))
				{
					auto width = 0u;
					auto height = 0u;
					auto& texture_manager = instance().service<ctexture_resource_manager_service>();

					const auto* material = material_manager.get(s.m_material);

					for (const auto& t : material->m_sampler_textures)
					{
						const auto* texture = texture_manager.get(t.m_texture);

						width = std::max(width, texture->m_image.m_width);
						height = std::max(height, texture->m_image.m_height);
					}

					auto u0 = s.m_source.x / width;
					auto u1 = (s.m_source.x + s.m_source.z) / width;
					auto v0 = s.m_source.y / height;
					auto v1 = (s.m_source.y + s.m_source.w) / height;
					s.m_uv0 = { u0, v0 };
					s.m_uv1 = { u1, v1 };
				}
			}
		}
	}

} //- kokoro::world::component

RTTR_REGISTRATION
{
	using namespace kokoro::world::component;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<ssprite>("ssprite")
		.custom_serialization()
		.prop("m_source", &ssprite::m_source)
		.prop("m_tint", &ssprite::m_tint)
		.prop("m_uv0", &ssprite::m_uv0)
		.prop("m_uv1", &ssprite::m_uv1)
		;
}