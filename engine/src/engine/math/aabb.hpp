#pragma once
#include <engine/math/vec2.hpp>
#include <box2d.h>

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	class caabb final
	{
	public:
		caabb() = default;
		~caabb() = default;
		caabb(const vec2_t& bottom_left, const vec2_t& top_right);
		caabb(float cx, float cy, float hw, float hh);

		operator						box2d::b2AABB() const { return m_aabb; }
		inline const box2d::b2AABB&		aabb() const { return m_aabb; }
		inline box2d::b2AABB&			aabb() { return m_aabb; }

	private:
		box2d::b2AABB m_aabb = { 0 };
	};

	using aabb_t = caabb;

} //- kokoro::math