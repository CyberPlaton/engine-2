#pragma once
#include <cstdint>

namespace kokoro::core::profile
{
	//------------------------------------------------------------------------------------------------------------------------
	class chighspeed_timer final
	{
	public:
		chighspeed_timer() = default;
		~chighspeed_timer() = default;

		void		start();
		void		stop();
		int64_t		duration() const;
		double		nanoseconds() const;

	private:
		int64_t m_start = 0;
		int64_t m_end = 0;
	};

} //- kokoro::core::profile