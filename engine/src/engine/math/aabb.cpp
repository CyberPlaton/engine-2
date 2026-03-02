#include <engine/math/aabb.hpp>

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	caabb::caabb(const vec2_t& bottom_left, const vec2_t& top_right)
	{
		m_aabb.lowerBound.x = bottom_left.x;
		m_aabb.lowerBound.y = bottom_left.y;
		m_aabb.upperBound.x = top_right.x;
		m_aabb.upperBound.y = top_right.y;
	}

	//------------------------------------------------------------------------------------------------------------------------
	caabb::caabb(float cx, float cy, float hw, float hh)
	{
		m_aabb.lowerBound.x = cx - hw;
		m_aabb.lowerBound.y = cy - hh;
		m_aabb.upperBound.x = cx + hw;
		m_aabb.upperBound.y = cy + hh;
	}

} //- kokoro::math