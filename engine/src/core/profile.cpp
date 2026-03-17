#include <core/profile.hpp>
#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <chrono>
#endif

namespace kokoro::core::profile
{
	//------------------------------------------------------------------------------------------------------------------------
	void chighspeed_timer::start()
	{
#if PLATFORM_WINDOWS
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		m_start = li.QuadPart;
#else
		m_start = std::chrono::high_resolution_clock::now();
#endif
		m_end = m_start;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void chighspeed_timer::stop()
	{
#if PLATFORM_WINDOWS
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		m_end = li.QuadPart;
#else
		m_end = std::chrono::high_resolution_clock::now();
#endif
	}

	//------------------------------------------------------------------------------------------------------------------------
	int64_t chighspeed_timer::duration() const
	{
#if PLATFORM_WINDOWS
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		return (int64_t)((m_end - m_start) * 1000000000 / freq.QuadPart);
#else
		return std::chrono::duration_cast<std::chrono::nanoseconds>(m_end - m_start).count();
#endif
	}

	//------------------------------------------------------------------------------------------------------------------------
	double chighspeed_timer::nanoseconds() const
	{
#if PLATFORM_WINDOWS
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		return static_cast<double>(m_end - m_start) * 1000000000.0 / static_cast<double>(freq.QuadPart);
#else
		return std::chrono::duration<double, std::nano>(m_end - m_start).count();
#endif
	}

} //- kokoro::core::profile