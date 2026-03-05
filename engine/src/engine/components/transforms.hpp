#pragma once
#include <engine/world/component.hpp>
#include <engine/world/world.hpp>
#include <engine/math/vec2.hpp>
#include <engine/math/mat4.hpp>
#include <engine/math/aabb.hpp>

namespace kokoro::world::component
{
	//------------------------------------------------------------------------------------------------------------------------
	struct slocal_transform final : public kokoro::ecs::icomponent
	{
		DECLARE_COMPONENT(slocal_transform);

		math::vec2_t m_position = { 0.0f, 0.0f };
		math::vec2_t m_scale = { 1.0f, 1.0f };
		math::vec2_t m_origin = { 0.0f, 0.0f };
		float m_rotation = 0.0f;
		
		RTTR_ENABLE(kokoro::ecs::icomponent);
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct sworld_transform final : public kokoro::ecs::icomponent
	{
		DECLARE_COMPONENT(sworld_transform);

		static void custom_serialize(const rttr::variant& var, nlohmann::json& json);
		static void custom_deserialize(rttr::variant& var, nlohmann::json& json);

		math::mat4_t m_matrix;	//- Final world space matrix after affine transformations and hierarchy transforms accumulation.
		math::aabb_t m_aabb;	//- Contains AABB in world space.

		RTTR_ENABLE(kokoro::ecs::icomponent);
	};

} //- kokoro::world::component