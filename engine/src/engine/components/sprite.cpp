#include <engine/components/sprite.hpp>
#include <registrator.hpp>

namespace kokoro::world::component
{
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
			{
				if (s.m_material && s.m_material->m_effect)
				{

				}
			}

			if (raylib::IsShaderValid(s.m_effect))
			{
				if (const auto path = render::effect::filepath(s.m_effect.id); !path.empty())
				{
					j["effect"] = path;
				}
			}
			if (raylib::IsTextureValid(s.m_texture))
			{
				if (const auto path = render::texture::filepath(s.m_texture.id); !path.empty())
				{
					j["texture"] = path;
				}
			}

			if (!j.contains("texture"))
			{
				j["texture"] = C_DEFAULT_TEXTURE;
			}
			if (!j.contains("effect"))
			{
				j["effect"] = C_DEFAULT_EFFECT;
			}

			j["source"] = core::io::to_json_object(s.m_source);
			j["tint"] = core::io::to_json_object(s.m_tint);
			j["layer"] = s.m_layer;
			j["uv0"] = core::io::to_json_object(s.m_uv0);
			j["uv1"] = core::io::to_json_object(s.m_uv1);

			json[type_name] = j;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ssprite::custom_deserialize(rttr::variant& var, nlohmann::json& json)
	{
		if (json.contains(core::io::C_OBJECT_TYPE_NAME))
		{
			const auto type_name = json[core::io::C_OBJECT_TYPE_NAME].get<std::string>();
			if (const auto type = rttr::type::get_by_name(type_name); type.is_valid() && type == rttr::type::get<ssprite>())
			{
				auto& j = json[type_name]; //- TODO: now this does look redundant and we probably should automate this part
				auto& s = var.get_value<ssprite>();

				auto effect_path = j["effect"].get<std::string>();
				auto texture_path = j["texture"].get<std::string>();

				if (const auto* effect = render::effect::create(effect_path); effect)
				{
					s.m_effect = *effect;
				}
				else
				{
					//- This is not the expected state, report a warning for this
					s.m_effect = *render::effect::create(C_DEFAULT_EFFECT);
				}

				if (const auto* texture = render::texture::create(texture_path); texture)
				{
					s.m_texture = *texture;
				}
				else
				{
					//- This is not the expected state, report a warning for this
					s.m_texture = *render::texture::create(C_DEFAULT_TEXTURE);
				}

				s.m_source = core::io::from_json_object(rttr::type::get<core::srect>(), j["source"]).get_value<core::srect>();
				s.m_tint = core::io::from_json_object(rttr::type::get<core::scolor>(), j["tint"]).get_value<core::scolor>();
				s.m_layer = j["layer"].get<uint8_t>();

				//- FIXME: add version update functionality for components
				const auto has_uv0 = j.contains("uv0");
				const auto has_uv1 = j.contains("uv1");

				if (has_uv0)
				{
					s.m_uv0 = core::io::from_json_object(rttr::type::get<vec2_t>(), j["uv0"]).get_value<vec2_t>();
				}
				if (has_uv1)
				{
					s.m_uv1 = core::io::from_json_object(rttr::type::get<vec2_t>(), j["uv1"]).get_value<vec2_t>();
				}

				//- Reconstruct uvs from source rectangle if any of them is missing
				if (!(has_uv0 && has_uv1))
				{
					auto u0 = s.m_source.x() / s.m_texture.width;
					auto u1 = (s.m_source.x() + s.m_source.w()) / s.m_texture.width;
					auto v0 = s.m_source.y() / s.m_texture.height;
					auto v1 = (s.m_source.y() + s.m_source.h()) / s.m_texture.height;
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