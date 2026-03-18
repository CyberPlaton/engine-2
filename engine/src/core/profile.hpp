#pragma once
#include <tracy.h>

#define CPU_ZONE
#define CPU_NAMED_ZONE(name)

#if PLATFORM_WINDOWS
	#if defined(PROFILE_ENABLE) && defined(TRACY_ENABLE)
		#undef CPU_ZONE
		#undef CPU_NAMED_ZONE
		#define CPU_ZONE ZoneScoped
		#define CPU_NAMED_ZONE(name) ZoneScopedN(name)
	#endif
#endif