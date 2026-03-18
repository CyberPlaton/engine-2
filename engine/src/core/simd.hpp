#pragma once
#include <cstdint>
#if SIMD_ENABLE
	#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
		#include <immintrin.h>
		#define KOKORO_SIMD_SSE
	#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
		#include <arm_neon.h>
		#define KOKORO_SIMD_NEON
	#endif
#else
#include <cstring>
#include <cmath>
#endif

#if defined(_MSC_VER)
	#define SIMDCALL __vectorcall
#else
	#define SIMDCALL __fastcall
#endif

namespace kokoro::core::simd
{
#if defined(KOKORO_SIMD_SSE)
	using v128_t = __m128;
#elif defined(KOKORO_SIMD_NEON)
	using v128_t = float32x4_t;
#else
	//- Fallback structure when SIMD is not supported.
	//------------------------------------------------------------------------------------------------------------------------
	struct v128_t final
	{
#if PLATFORM_WINDOWS
		__declspec(align(16)) float data[4] = { 0 };
#else
		__attribute__((aligned(16))) float data[4] = { 0 };
#endif
	};
#endif

	//- Load 4 float values into a SIMD register
	//------------------------------------------------------------------------------------------------------------------------
	inline v128_t SIMDCALL		load(const float* source)
	{
#if defined(KOKORO_SIMD_SSE)
		return _mm_loadu_ps(source);
#elif defined(KOKORO_SIMD_NEON)
		return vld1q_f32(source);
#else
		v128_t output;
		std::memcpy(output.data, source, sizeof(float) * 4);
		return output;
#endif
	}

	//- Store data from SIMD register into 4 float values
	//------------------------------------------------------------------------------------------------------------------------
	inline void SIMDCALL		store(v128_t source, float* target)
	{
#if defined(KOKORO_SIMD_SSE)
		_mm_storeu_ps(target, source);
#elif defined(KOKORO_SIMD_NEON)
		vst1q_f32(target, source);
#else
		std::memcpy(target, &source.data, sizeof(float) * 4);
#endif
	}

	//- Initialize SIMD register from a single float value
	//------------------------------------------------------------------------------------------------------------------------
	inline v128_t SIMDCALL		set1(const float source)
	{
#if defined(KOKORO_SIMD_SSE)
		return _mm_set1_ps(source);
#elif defined(KOKORO_SIMD_NEON)
		return vdupq_n_f32(source);
#else
		v128_t output;
		output.data[0] = source;
		output.data[1] = source;
		output.data[2] = source;
		output.data[3] = source;
		return output;
#endif
	}

	//- a + b
	//------------------------------------------------------------------------------------------------------------------------
	inline v128_t SIMDCALL		add(v128_t a, v128_t b)
	{
#if defined(KOKORO_SIMD_SSE)
		return _mm_add_ps(a, b);
#elif defined(KOKORO_SIMD_NEON)
		return vaddq_f32(a, b);
#else
		v128_t output;
		for (auto i = 0; i < 4; ++i)
		{
			output.data[i] = a.data[i] + b.data[i];
		}
		return output;
#endif
	}

	//- a - b
	//------------------------------------------------------------------------------------------------------------------------
	inline v128_t SIMDCALL		sub(v128_t a, v128_t b)
	{
#if defined(KOKORO_SIMD_SSE)
		return _mm_sub_ps(a, b);
#elif defined(KOKORO_SIMD_NEON)
		return vsubq_f32(a, b);
#else
		v128_t output;
		for (auto i = 0; i < 4; ++i)
		{
			output.data[i] = a.data[i] - b.data[i];
		}
		return output;
#endif
	}

	//- a * b
	//------------------------------------------------------------------------------------------------------------------------
	inline v128_t SIMDCALL		mul(v128_t a, v128_t b)
	{
#if defined(KOKORO_SIMD_SSE)
		return _mm_mul_ps(a, b);
#elif defined(KOKORO_SIMD_NEON)
		return vmulq_f32(a, b);
#else
		v128_t output;
		for (auto i = 0; i < 4; ++i)
		{
			output.data[i] = a.data[i] * b.data[i];
		}
		return output;
#endif
	}

	//- a / b
	//------------------------------------------------------------------------------------------------------------------------
	inline v128_t SIMDCALL		div(v128_t a, v128_t b)
	{
#if defined(KOKORO_SIMD_SSE)
		return _mm_div_ps(a, b);
#elif defined(KOKORO_SIMD_NEON)
		return vdivq_f32(a, b);
#else
		v128_t output;
		for (auto i = 0; i < 4; ++i)
		{
			output.data[i] = a.data[i] / b.data[i];
		}
		return output;
#endif
	}

	//- (a * b) + c
	//------------------------------------------------------------------------------------------------------------------------
	inline v128_t SIMDCALL		madd(v128_t a, v128_t b, v128_t c)
	{
#if defined(KOKORO_SIMD_SSE)
		return _mm_add_ps(_mm_mul_ps(a, b), c);
#elif defined(KOKORO_SIMD_NEON)
		return vmlaq_f32(c, a, b);
#else
		v128_t output;
		for (auto i = 0; i < 4; ++i)
		{
			output.data[i] = (a.data[i] * b.data[i]) + c.data[i];
		}
		return output;
#endif
	}

} //- kokoro::core::simd