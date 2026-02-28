#pragma once

namespace kokoro::math
{
	//------------------------------------------------------------------------------------------------------------------------
	struct svec2 final
	{
		svec2() = default;
		constexpr svec2(float x, float y) : x(x), y(y) {}
		constexpr svec2(float s) : x(s), y(s) {}

		float length() const;
		float distance(const svec2& rh) const;

		float x = 0.0f;
		float y = 0.0f;
	};

	using vec2_t = svec2;

	inline svec2 operator+(const svec2& a, const svec2& b) { return { a.x + b.x, a.y + b.y }; }
	inline svec2 operator-(const svec2& a, const svec2& b) { return { a.x - b.x, a.y - b.y }; }
	inline svec2 operator*(const svec2& a, const svec2& b) { return { a.x * b.x, a.y * b.y }; }
	inline svec2 operator/(const svec2& a, const svec2& b) { return { a.x / b.x, a.y / b.y }; }
	inline svec2 operator+(const svec2& a, float s) { return { a.x + s, a.y + s }; }
	inline svec2 operator-(const svec2& a, float s) { return { a.x - s, a.y - s }; }
	inline svec2 operator*(const svec2& a, float s) { return { a.x * s, a.y * s }; }
	inline svec2 operator/(const svec2& a, float s) { return { a.x / s, a.y / s }; }
	inline svec2& operator+=(svec2& a, const svec2& b) { a.x += b.x; a.y += b.y; return a; }
	inline svec2& operator-=(svec2& a, const svec2& b) { a.x -= b.x; a.y -= b.y; return a; }
	inline svec2& operator*=(svec2& a, const svec2& b) { a.x *= b.x; a.y *= b.y; return a; }
	inline svec2& operator/=(svec2& a, const svec2& b) { a.x /= b.x; a.y /= b.y; return a; }

} //- kokoro::math