#pragma once
#include <cstdint>

#define KOKORO_EMBEDDED_SHADERS_LIST \
ENTRY(vs_default) \
ENTRY(vs_postprocess) \
ENTRY(ps_default) \
ENTRY(ps_postprocess_sepia) \
ENTRY(ps_postprocess_blur) \
ENTRY(ps_postprocess_grayscale) \
ENTRY(ps_postprocess_vignette) \
ENTRY(ps_postprocess_filmgrain) \
ENTRY(ps_postprocess_chromatic_aberration) \
ENTRY(ps_postprocess_bloom) \
ENTRY(ps_postprocess_scanlines) \
ENTRY(ps_postprocess_sharpen) \
ENTRY(ps_postprocess_posterize) \
ENTRY(ps_postprocess_invert)

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