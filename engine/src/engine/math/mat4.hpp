#pragma once
#include <engine/math/vec4.hpp>
#include <engine/math/vec3.hpp>

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	struct smat4x4 final
	{
		smat4x4() = default;
		constexpr smat4x4(float s) : value{ s, 0.0f, 0.0f, 0.0f, 0.0f, s, 0.0f, 0.0f, 0.0f, 0.0f, s, 0.0f, 0.0f, 0.0f, 0.0f, s } {}

		smat4x4		inverse() const;
		smat4x4&	translate(const vec3_t& v);
		smat4x4&	scale(const vec3_t& v);
		smat4x4&	rotate(const vec3_t& v);

		float* operator[](size_t i) { return &value[i * 4]; }
		constexpr const float* operator[](size_t i) const { return &value[i * 4]; }

		float value[16] = { 0 };
	};

	using mat4_t = smat4x4;

	//------------------------------------------------------------------------------------------------------------------------
	inline smat4x4 operator*(const smat4x4& a, const smat4x4& b)
	{
		smat4x4 result;

		for (int col = 0; col < 4; ++col)
		{
			for (int row = 0; row < 4; ++row)
			{
				result.value[col * 4 + row] =
					a.value[0 * 4 + row] * b.value[col * 4 + 0] +
					a.value[1 * 4 + row] * b.value[col * 4 + 1] +
					a.value[2 * 4 + row] * b.value[col * 4 + 2] +
					a.value[3 * 4 + row] * b.value[col * 4 + 3];
			}
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------------------------------
	inline vec4_t operator*(const smat4x4& m, const vec4_t& v)
	{
		vec4_t result;
		result.x = m.value[0] * v.x + m.value[4] * v.y + m.value[8] * v.z + m.value[12] * v.w;
		result.y = m.value[1] * v.x + m.value[5] * v.y + m.value[9] * v.z + m.value[13] * v.w;
		result.z = m.value[2] * v.x + m.value[6] * v.y + m.value[10] * v.z + m.value[14] * v.w;
		result.w = m.value[3] * v.x + m.value[7] * v.y + m.value[11] * v.z + m.value[15] * v.w;

		return result;
	}

	//------------------------------------------------------------------------------------------------------------------------
	inline vec3_t operator*(const smat4x4& m, const vec3_t& v)
	{
		vec3_t result;
		result.x = m.value[0] * v.x + m.value[4] * v.y + m.value[8] * v.z + m.value[12] * 1.0f;
		result.y = m.value[1] * v.x + m.value[5] * v.y + m.value[9] * v.z + m.value[13] * 1.0f;
		result.z = m.value[2] * v.x + m.value[6] * v.y + m.value[10] * v.z + m.value[14] * 1.0f;
		return result;
	}

} //- kokoro::math