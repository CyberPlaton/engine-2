#pragma once
#include <engine/math/vec2.hpp>
#include <box2d.h>

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	struct sray
	{
		operator box2d::b2RayCastInput() const
		{
			return { {m_origin.x, m_origin.y}, {m_direction.x, m_direction.y}, m_max_fraction };
		}

		vec2_t m_origin;
		vec2_t m_direction;
		float m_max_fraction = 1.0f;
	};

	using ray_t = sray;

} //- kokoro::math