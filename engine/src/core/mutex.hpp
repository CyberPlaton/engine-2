#pragma once
#include <cstdint>

namespace kokoro::core
{
	//------------------------------------------------------------------------------------------------------------------------
	class cmutex final
	{
	public:
		cmutex();
		~cmutex();

		void	lock();
		void	unlock();

	private:
#if PLATFORM_WINDOWS
		__declspec(align(16)) uint8_t m_internal[64];
#else
		__attribute__((aligned(16))) uint8_t m_internal[64];
#endif
	};

	//------------------------------------------------------------------------------------------------------------------------
	class cscoped_mutex final
	{
	public:
		cscoped_mutex(cmutex& m);
		~cscoped_mutex();
		cscoped_mutex& operator=(const cscoped_mutex&) = delete;
		cscoped_mutex(const cscoped_mutex&) = delete;
		cscoped_mutex(cscoped_mutex&&) = delete;
		cscoped_mutex& operator=(const cscoped_mutex&&) = delete;

	private:
		cmutex& m_mutex;
	};

} //- kokoro::core