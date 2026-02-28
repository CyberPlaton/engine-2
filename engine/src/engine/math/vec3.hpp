#pragma once

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	struct svec3 final
	{
		svec3() = default;
		constexpr svec3(float x, float y, float z) : x(x), y(y), z(z) {}
		constexpr svec3(float s) : x(s), y(s), z(s) {}

		svec3 normalize() const;
		float dot(const svec3& rh) const;
		svec3 cross(const svec3& rh) const;
		float length() const;
		float distance(const svec3& rh) const;

		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};

	using vec3_t = svec3;

	inline svec3 operator+(const svec3& a, const svec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
	inline svec3 operator-(const svec3& a, const svec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
	inline svec3 operator*(const svec3& a, const svec3& b) { return { a.x * b.x, a.y * b.y, a.z * b.z }; }
	inline svec3 operator/(const svec3& a, const svec3& b) { return { a.x / b.x, a.y / b.y, a.z / b.z }; }
	inline svec3 operator+(const svec3& a, float s) { return { a.x + s, a.y + s, a.z + s }; }
	inline svec3 operator-(const svec3& a, float s) { return { a.x - s, a.y - s, a.z - s }; }
	inline svec3 operator*(const svec3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
	inline svec3 operator/(const svec3& a, float s) { return { a.x / s, a.y / s, a.z / s }; }
	inline svec3& operator+=(svec3& a, const svec3& b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
	inline svec3& operator-=(svec3& a, const svec3& b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; return a; }
	inline svec3& operator*=(svec3& a, const svec3& b) { a.x *= b.x; a.y *= b.y; a.z *= b.z; return a; }
	inline svec3& operator/=(svec3& a, const svec3& b) { a.x /= b.x; a.y /= b.y; a.z /= b.z; return a; }

} //- kokoro::math