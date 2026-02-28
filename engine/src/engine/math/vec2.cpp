#include <engine/math/vec2.hpp>
#include <registrator.hpp>

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	float svec2::length() const
	{
		return std::sqrt(x * x + y * y);
	}

	//------------------------------------------------------------------------------------------------------------------------
	float svec2::distance(const svec2& rh) const
	{
		const auto dx = x - rh.x;
		const auto dy = y - rh.y;
		return std::sqrt(dx * dx + dy * dy);
	}

} //- kokoro::math

RTTR_REGISTRATION
{
	using namespace kokoro::math;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<vec2_t>("vec2_t")
		.prop("x", &vec2_t::x)
		.prop("y", &vec2_t::y)
		;
}