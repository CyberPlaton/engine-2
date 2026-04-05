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

		struct ssubpass
		{
			std::vector<suniform> m_uniforms;
			std::vector<sampler_texture_t> m_sampler_textures;
			std::string m_effect;
			uint64_t m_blending = 0;
			uint64_t m_state = 0;
			backbuffer_ratio_t m_backbuffer_ratio = backbuffer_ratio_t::Equal;
		};

		std::vector<std::string> m_predecessors;
		std::vector<std::string> m_successors;
		std::vector<ssubpass> m_passes;
		std::string m_name;
	};

	//- Definition of one rendering pass for a post process. A post process can consist of n-many passes.
	//------------------------------------------------------------------------------------------------------------------------
	struct spostprocess_subpass final
	{
		using sampler_texture_t = smaterial::ssampler_texture;
		inline static constexpr int8_t C_CHAIN_FRAMEBUFFER = -1;
		inline static constexpr uint64_t C_BLEND_DEFAULT = 0;
		inline static constexpr uint64_t C_STATE_DEFAULT = 0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_CULL_CW;

		std::vector<suniform> m_uniforms;									//- Custom uniform binding data to be bound for shading
		std::vector<sampler_texture_t> m_sampler_textures;					//- Additional sampler textures to be bound for shading
		suniform m_framebuffer_sampler;										//- Output framebuffer sampler uniform
		cview<seffect> m_effect;											//- Effect to be used for shading
		uint64_t m_blending = C_BLEND_DEFAULT;
		uint64_t m_state = C_STATE_DEFAULT;
		backbuffer_ratio_t m_backbuffer_ratio = backbuffer_ratio_t::Equal;	//- Size of the output framebuffer in relation to the current backbuffer
		bgfx::FrameBufferHandle m_output_framebuffer = BGFX_INVALID_HANDLE;	//- Target to render to
		uint16_t m_x = 0;
		uint16_t m_y = 0;
		int8_t m_input_index = C_CHAIN_FRAMEBUFFER;							//- Which subpasses output to take as input for this subpass,
																			//- if value is C_CHAIN_FRAMEBUFFER then the input will be the
																			//- the "chain input", i.e. the output of previous logical
																			//- postprocess.
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct spostprocess
	{
		static std::optional<spostprocess>	load(const rttr::variant& snapshot);
		static void							unload(spostprocess& postprocess);

		std::vector<std::string> m_predecessors;							//- Postprocesses that, if enabled, must come before this one
		std::vector<std::string> m_successors;								//- Postprocesses that, if enabled, must come after this one
		std::vector<spostprocess_subpass> m_passes;							//- Passes to be executed for this postprocess
		std::string m_name;
	};

} //- kokoro