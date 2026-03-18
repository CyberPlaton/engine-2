#include <engine/math/mat4.hpp>
#include <core/profile.hpp>

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	smat4x4 smat4x4::inverse() const
	{
		const float* m = value;

		smat4x4 inv;
		float* invOut = inv.value;

		invOut[0] = m[5] * m[10] * m[15] -
			m[5] * m[11] * m[14] -
			m[9] * m[6] * m[15] +
			m[9] * m[7] * m[14] +
			m[13] * m[6] * m[11] -
			m[13] * m[7] * m[10];

		invOut[4] = -m[4] * m[10] * m[15] +
			m[4] * m[11] * m[14] +
			m[8] * m[6] * m[15] -
			m[8] * m[7] * m[14] -
			m[12] * m[6] * m[11] +
			m[12] * m[7] * m[10];

		invOut[8] = m[4] * m[9] * m[15] -
			m[4] * m[11] * m[13] -
			m[8] * m[5] * m[15] +
			m[8] * m[7] * m[13] +
			m[12] * m[5] * m[11] -
			m[12] * m[7] * m[9];

		invOut[12] = -m[4] * m[9] * m[14] +
			m[4] * m[10] * m[13] +
			m[8] * m[5] * m[14] -
			m[8] * m[6] * m[13] -
			m[12] * m[5] * m[10] +
			m[12] * m[6] * m[9];

		invOut[1] = -m[1] * m[10] * m[15] +
			m[1] * m[11] * m[14] +
			m[9] * m[2] * m[15] -
			m[9] * m[3] * m[14] -
			m[13] * m[2] * m[11] +
			m[13] * m[3] * m[10];

		invOut[5] = m[0] * m[10] * m[15] -
			m[0] * m[11] * m[14] -
			m[8] * m[2] * m[15] +
			m[8] * m[3] * m[14] +
			m[12] * m[2] * m[11] -
			m[12] * m[3] * m[10];

		invOut[9] = -m[0] * m[9] * m[15] +
			m[0] * m[11] * m[13] +
			m[8] * m[1] * m[15] -
			m[8] * m[3] * m[13] -
			m[12] * m[1] * m[11] +
			m[12] * m[3] * m[9];

		invOut[13] = m[0] * m[9] * m[14] -
			m[0] * m[10] * m[13] -
			m[8] * m[1] * m[14] +
			m[8] * m[2] * m[13] +
			m[12] * m[1] * m[10] -
			m[12] * m[2] * m[9];

		invOut[2] = m[1] * m[6] * m[15] -
			m[1] * m[7] * m[14] -
			m[5] * m[2] * m[15] +
			m[5] * m[3] * m[14] +
			m[13] * m[2] * m[7] -
			m[13] * m[3] * m[6];

		invOut[6] = -m[0] * m[6] * m[15] +
			m[0] * m[7] * m[14] +
			m[4] * m[2] * m[15] -
			m[4] * m[3] * m[14] -
			m[12] * m[2] * m[7] +
			m[12] * m[3] * m[6];

		invOut[10] = m[0] * m[5] * m[15] -
			m[0] * m[7] * m[13] -
			m[4] * m[1] * m[15] +
			m[4] * m[3] * m[13] +
			m[12] * m[1] * m[7] -
			m[12] * m[3] * m[5];

		invOut[14] = -m[0] * m[5] * m[14] +
			m[0] * m[6] * m[13] +
			m[4] * m[1] * m[14] -
			m[4] * m[2] * m[13] -
			m[12] * m[1] * m[6] +
			m[12] * m[2] * m[5];

		invOut[3] = -m[1] * m[6] * m[11] +
			m[1] * m[7] * m[10] +
			m[5] * m[2] * m[11] -
			m[5] * m[3] * m[10] -
			m[9] * m[2] * m[7] +
			m[9] * m[3] * m[6];

		invOut[7] = m[0] * m[6] * m[11] -
			m[0] * m[7] * m[10] -
			m[4] * m[2] * m[11] +
			m[4] * m[3] * m[10] +
			m[8] * m[2] * m[7] -
			m[8] * m[3] * m[6];

		invOut[11] = -m[0] * m[5] * m[11] +
			m[0] * m[7] * m[9] +
			m[4] * m[1] * m[11] -
			m[4] * m[3] * m[9] -
			m[8] * m[1] * m[7] +
			m[8] * m[3] * m[5];

		invOut[15] = m[0] * m[5] * m[10] -
			m[0] * m[6] * m[9] -
			m[4] * m[1] * m[10] +
			m[4] * m[2] * m[9] +
			m[8] * m[1] * m[6] -
			m[8] * m[2] * m[5];

		float det = m[0] * invOut[0] + m[1] * invOut[4] + m[2] * invOut[8] + m[3] * invOut[12];

		if (det == 0) return {};

		det = 1.0f / det;

		for (int i = 0; i < 16; i++)
			invOut[i] = invOut[i] * det;

		return inv;
	}

	//------------------------------------------------------------------------------------------------------------------------
	smat4x4& smat4x4::translate(const vec3_t& v)
	{
		CPU_ZONE;

#if SIMD_ENABLE
		auto column0 = core::simd::load(&value[0]);
		auto column1 = core::simd::load(&value[4]);
		auto column2 = core::simd::load(&value[8]);
		auto column3 = core::simd::load(&value[12]);
		auto x = core::simd::set1(v.x);
		auto y = core::simd::set1(v.y);
		auto z = core::simd::set1(v.z);

		auto rx = core::simd::mul(column0, x);
		auto ry = core::simd::mul(column1, y);
		auto rz = core::simd::mul(column2, z);

		auto rxyz = core::simd::add(core::simd::add(core::simd::add(rx, ry), rz), column3);

		core::simd::store(rxyz, &value[12]);
#else
		value[12] += value[0] * v.x + value[4] * v.y + value[8] * v.z;
		value[13] += value[1] * v.x + value[5] * v.y + value[9] * v.z;
		value[14] += value[2] * v.x + value[6] * v.y + value[10] * v.z;
		value[15] += value[3] * v.x + value[7] * v.y + value[11] * v.z;
#endif
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	smat4x4& smat4x4::scale(const vec3_t& v)
	{
		CPU_ZONE;

#if SIMD_ENABLE
		auto column0 = core::simd::load(&value[0]);
		auto column1 = core::simd::load(&value[4]);
		auto column2 = core::simd::load(&value[8]);
		auto x = core::simd::set1(v.x);
		auto y = core::simd::set1(v.y);
		auto z = core::simd::set1(v.z);

		core::simd::store(core::simd::mul(column0, x), &value[0]);
		core::simd::store(core::simd::mul(column1, y), &value[4]);
		core::simd::store(core::simd::mul(column2, z), &value[8]);
#else
		value[0] *= v.x;
		value[1] *= v.x;
		value[2] *= v.x;
		value[3] *= v.x;

		value[4] *= v.y;
		value[5] *= v.y;
		value[6] *= v.y;
		value[7] *= v.y;

		value[8] *= v.z;
		value[9] *= v.z;
		value[10] *= v.z;
		value[11] *= v.z;
#endif
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	smat4x4& smat4x4::rotate(const vec3_t& v)
	{
		CPU_ZONE;

#if SIMD_ENABLE
		if (v.x != 0.0f)
		{
			const core::simd::v128_t angle = core::simd::set1(v.x);
			const core::simd::v128_t c = core::simd::cos(angle);
			const core::simd::v128_t s = core::simd::sin(angle);
			const core::simd::v128_t neg_s = core::simd::mul(s, core::simd::set1(-1.0f));

			core::simd::v128_t col1 = core::simd::load(&value[4]);
			core::simd::v128_t col2 = core::simd::load(&value[8]);

			core::simd::store(core::simd::madd(col1, c, core::simd::mul(col2, s)), &value[4]);
			core::simd::store(core::simd::madd(col1, neg_s, core::simd::mul(col2, c)), &value[8]);
		}

		if (v.y != 0.0f)
		{
			const core::simd::v128_t angle = core::simd::set1(v.y);
			const core::simd::v128_t c = core::simd::cos(angle);
			const core::simd::v128_t s = core::simd::sin(angle);
			const core::simd::v128_t neg_s = core::simd::mul(s, core::simd::set1(-1.0f));

			core::simd::v128_t col0 = core::simd::load(&value[0]);
			core::simd::v128_t col2 = core::simd::load(&value[8]);

			core::simd::store(core::simd::madd(col0, c, core::simd::mul(col2, neg_s)), &value[0]);
			core::simd::store(core::simd::madd(col0, s, core::simd::mul(col2, c)), &value[8]);
		}

		if (v.z != 0.0f)
		{
			const core::simd::v128_t angle = core::simd::set1(v.z);
			const core::simd::v128_t c = core::simd::cos(angle);
			const core::simd::v128_t s = core::simd::sin(angle);
			const core::simd::v128_t neg_s = core::simd::mul(s, core::simd::set1(-1.0f));

			core::simd::v128_t col0 = core::simd::load(&value[0]);
			core::simd::v128_t col1 = core::simd::load(&value[4]);

			core::simd::store(core::simd::madd(col0, c, core::simd::mul(col1, s)), &value[0]);
			core::simd::store(core::simd::madd(col0, neg_s, core::simd::mul(col1, c)), &value[4]);
		}
#else
		if (v.x != 0.0f)
		{

			const auto c = cosf(v.x);
			const auto s = sinf(v.x);
			for (int i = 0; i < 4; ++i)
			{
				float v1 = value[4 + i];
				float v2 = value[8 + i];
				value[4 + i] = v1 * c + v2 * s;
				value[8 + i] = v1 * -s + v2 * c;
			}
		}
		if (v.y != 0.0f)
		{
			const auto c = cosf(v.y);
			const auto s = sinf(v.y);
			for (int i = 0; i < 4; ++i)
			{
				float v0 = value[i];
				float v2 = value[8 + i];
				value[i] = v0 * c + v2 * -s;
				value[8 + i] = v0 * s + v2 * c;
			}
		}
		if (v.z != 0.0f)
		{
			const auto c = cosf(v.z);
			const auto s = sinf(v.z);
			for (int i = 0; i < 4; ++i)
			{
				float v0 = value[i];
				float v1 = value[4 + i];
				value[i] = v0 * c + v1 * s;
				value[4 + i] = v0 * -s + v1 * c;
			}
		}
#endif
		return *this;
	}

} //- kokoro::math