#include <engine/math/vec3.hpp>
#include <registrator.hpp>

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	svec3 svec3::normalize() const
	{
		float len = std::sqrt(x * x + y * y + z * z);
		if (len == 0.0f) return { 0.0f, 0.0f, 0.0f };
		return { x / len, y / len, z / len };
	}

	//------------------------------------------------------------------------------------------------------------------------
	float svec3::dot(const svec3& rh) const
	{
		return x * rh.x + y * rh.y + z * rh.z;
	}

	//------------------------------------------------------------------------------------------------------------------------
	svec3 svec3::cross(const svec3& rh) const
	{
		return
		{
			y * rh.z - z * rh.y,
			z * rh.x - x * rh.z,
			x * rh.y - y * rh.x
		};
	}

	//------------------------------------------------------------------------------------------------------------------------
	float svec3::length() const
	{
		return std::sqrt(x * x + y * y + z * z);
	}

	//------------------------------------------------------------------------------------------------------------------------
	float svec3::distance(const svec3& rh) const
	{
		const auto dx = x - rh.x;
		const auto dy = y - rh.y;
		const auto dz = z - rh.z;
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

} //- kokoro::math

RTTR_REGISTRATION
{
	using namespace kokoro::math;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<vec3_t>("vec3_t")
		.prop("x", &vec3_t::x)
		.prop("y", &vec3_t::y)
		.prop("z", &vec3_t::z)
		;
}