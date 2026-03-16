#pragma once

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	struct
#if PLATFORM_WINDOWS
		__declspec(align(16))
#else
		__attribute__((aligned(16)))
#endif
		svec4 final
	{
		svec4() = default;
		constexpr svec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
		constexpr svec4(float s) : x(s), y(s), z(s), w(s) {}

		float length() const;
		float distance(const svec4& rh) const;

		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 0.0f;
	};

	using vec4_t = svec4;

	inline svec4 operator+(const svec4& a, const svec4& b) { return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
	inline svec4 operator-(const svec4& a, const svec4& b) { return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
	inline svec4 operator*(const svec4& a, const svec4& b) { return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w }; }
	inline svec4 operator/(const svec4& a, const svec4& b) { return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }; }
	inline svec4 operator+(const svec4& a, float s) { return { a.x + s, a.y + s, a.z + s, a.w + s }; }
	inline svec4 operator-(const svec4& a, float s) { return { a.x - s, a.y - s, a.z - s, a.w - s }; }
	inline svec4 operator*(const svec4& a, float s) { return { a.x * s, a.y * s, a.z * s, a.w * s }; }
	inline svec4 operator/(const svec4& a, float s) { return { a.x / s, a.y / s, a.z / s, a.w / s }; }
	inline svec4& operator+=(svec4& a, const svec4& b) { a.x += b.x; a.y += b.y; a.z += b.z; a.w += b.w; return a; }
	inline svec4& operator-=(svec4& a, const svec4& b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; a.w -= b.w; return a; }
	inline svec4& operator*=(svec4& a, const svec4& b) { a.x *= b.x; a.y *= b.y; a.z *= b.z; a.w *= b.w; return a; }
	inline svec4& operator/=(svec4& a, const svec4& b) { a.x /= b.x; a.y /= b.y; a.z /= b.z; a.w /= b.w; return a; }

} //- kokoro::math