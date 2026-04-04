#pragma once
#include <engine/effect/effect.hpp>
#include <engine/material/material.hpp>
#include <bgfx.h>
#include <rttr.h>
#include <string>
#include <vector>
#include <optional>

namespace kokoro
{
	using backbuffer_ratio_t = bgfx::BackbufferRatio::Enum;

	//------------------------------------------------------------------------------------------------------------------------
	struct spostprocess_snapshot final
	{
		using sampler_texture_t = smaterial_snapshot::ssampler_texture;

		std::vector<std::string> m_predecessors;
		std::vector<std::string> m_successors;
		std::vector<suniform> m_uniforms;
		std::vector<sampler_texture_t> m_sampler_textures;
		std::string m_name;
		std::string m_effect;
		uint64_t m_blending = 0;
		uint64_t m_state = 0;
		backbuffer_ratio_t m_backbuffer_ratio = backbuffer_ratio_t::Equal;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct spostprocess
	{
		using sampler_texture_t = smaterial::ssampler_texture;

		inline static constexpr uint64_t C_POSTPROCESS_STATE_DEFAULT = 0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_CULL_CW
			;

		inline static constexpr uint64_t C_POSTPROCESS_BLEND_DEFAULT = 0
			;

		static std::optional<spostprocess>	load(const rttr::variant& snapshot);
		static void							unload(spostprocess& postprocess);

		std::vector<std::string> m_predecessors;							//- Postprocesses that, if enabled, must come before this one
		std::vector<std::string> m_successors;								//- Postprocesses that, if enabled, must come after this one
		std::vector<suniform> m_uniforms;									//- Custom uniform binding data to be bound for shading
		std::vector<sampler_texture_t> m_sampler_textures;					//- Additional sampler textures to be bound for shading
		suniform m_framebuffer_sampler;										//- Output framebuffer sampler uniform
		std::string m_name;
		uint64_t m_blending = C_POSTPROCESS_STATE_DEFAULT;
		uint64_t m_state = C_POSTPROCESS_BLEND_DEFAULT;
		cview<seffect> m_effect;											//- Effect to be used for shading
		backbuffer_ratio_t m_backbuffer_ratio = backbuffer_ratio_t::Equal;
		bgfx::FrameBufferHandle m_output_framebuffer = BGFX_INVALID_HANDLE;	//- Target to render this postprocess to
		uint16_t m_x = 0;
		uint16_t m_y = 0;
	};

} //- kokoro