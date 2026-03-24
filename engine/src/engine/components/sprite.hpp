#pragma once
#include <engine/world/component.hpp>
#include <engine/world/world.hpp>
#include <engine/render/mesh.hpp>
#include <engine/material/material.hpp>
#include <engine/math/vec4.hpp>
#include <engine/math/vec2.hpp>

namespace kokoro::world::component
{
	//------------------------------------------------------------------------------------------------------------------------
	struct ssprite final : public kokoro::ecs::icomponent
	{
		DECLARE_COMPONENT(ssprite);

		static void custom_serialize(const rttr::variant& var, nlohmann::json& json);
		static void custom_deserialize(rttr::variant& var, nlohmann::json& json);

		math::vec4_t m_source = { 0.0f, 0.0f, 1.0f, 1.0f };
		math::vec2_t m_uv0;
		math::vec2_t m_uv1;
		cview<smesh> m_mesh;
		cview<smaterial> m_material;
		uint32_t m_tint = 0;

		RTTR_ENABLE(kokoro::ecs::icomponent);
	};

} //- kokoro::world::component