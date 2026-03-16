#include <engine/math/vec4.hpp>
#include <core/registrator.hpp>

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	float svec4::length() const
	{
		return std::sqrt(x * x + y * y + z * z + w * w);
	}

	//------------------------------------------------------------------------------------------------------------------------
	float svec4::distance(const svec4& rh) const
	{
		const auto dx = x - rh.x;
		const auto dy = y - rh.y;
		const auto dz = z - rh.z;
		const auto dw = w - rh.w;
		return std::sqrt(dx * dx + dy * dy + dz * dz + dw * dw);
	}

} //- kokoro::math

RTTR_REGISTRATION
{
	using namespace kokoro::math;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<vec4_t>("vec4_t")
		.prop("x", &vec4_t::x)
		.prop("y", &vec4_t::y)
		.prop("z", &vec4_t::z)
		.prop("w", &vec4_t::w)
		;
}