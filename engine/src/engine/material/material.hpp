#pragma once
#include <engine/effect/effect.hpp>
#include <engine/render/texture.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	struct smaterial_snapshot final
	{

	};

	//------------------------------------------------------------------------------------------------------------------------
	struct smaterial final
	{
		struct ssampler_texture
		{
			const stexture* m_texture = nullptr;
			uint64_t m_sampler_flags = 0;
			uint8_t m_sampler_stage = 0;
		};

		std::vector<ssampler_texture> m_sampler_textures;
		seffect* m_effect = nullptr;
		uint64_t m_blending = 0;
		uint64_t m_state = 0;
	};



} //- kokoro