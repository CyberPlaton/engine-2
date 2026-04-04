#pragma once
#include <cstdint>

#define KOKORO_EMBEDDED_SHADERS_LIST \
ENTRY(vs_default) \
ENTRY(vs_postprocess) \
ENTRY(ps_default) \
ENTRY(ps_sepia) \
ENTRY(ps_blur) \
ENTRY(ps_grayscale) \
ENTRY(ps_vignette) \
ENTRY(ps_filmgrain) \
ENTRY(ps_chromatic_aberration) \
ENTRY(ps_bloom) \
ENTRY(ps_scanlines) \
ENTRY(ps_sharpen) \
ENTRY(ps_posterize) \
ENTRY(ps_invert) \
ENTRY(ps_count)

namespace kokoro
{
	//- Containing shaders integrated into the engine
	//------------------------------------------------------------------------------------------------------------------------
	struct sembedded_shaders final
	{
		enum type : uint8_t
		{
#define ENTRY(name) name,
			KOKORO_EMBEDDED_SHADERS_LIST
#undef ENTRY
		};

		static const char*	get(type type_name);
		static const char*	get(const char* name);
	};

} //- kokoro