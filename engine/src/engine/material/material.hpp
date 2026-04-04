#pragma once
#include <engine/effect/effect.hpp>
#include <engine/render/texture.hpp>
#include <engine/services/resource_manager_service.hpp>
#include <optional>

namespace kokoro
{
	using uniform_type_t = bgfx::UniformType::Enum;

	//------------------------------------------------------------------------------------------------------------------------
	struct suniform final
	{
		std::string m_name;
		rttr::variant m_data;
		uniform_type_t m_type = uniform_type_t::Count;
		bgfx::UniformHandle m_handle = BGFX_INVALID_HANDLE;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct smaterial_snapshot final
	{
		struct ssampler_texture
		{
			std::string m_texture;
			uint64_t m_sampler_flags = 0;
			uint8_t m_sampler_stage = 0;
		};

		std::vector<ssampler_texture> m_sampler_textures;
		std::vector<suniform> m_uniforms;
		std::string m_effect;
		uint64_t m_blending = 0;
		uint64_t m_state = 0;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct smaterial
	{
		inline static constexpr uint64_t C_MATERIAL_STATE_DEFAULT = 0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CW
			| BGFX_STATE_MSAA
			;

		inline static constexpr uint64_t C_MATERIAL_BLEND_DEFAULT = 0
			| BGFX_STATE_BLEND_NORMAL
			;

		struct ssampler_texture
		{
			cview<stexture> m_texture;
			uint64_t m_sampler_flags = 0;
			uint8_t m_sampler_stage = 0;
		};

		static std::optional<smaterial>	load(const rttr::variant& snapshot);
		static void						unload(smaterial& material);

		std::vector<ssampler_texture> m_sampler_textures;
		std::vector<suniform> m_uniforms;
		cview<seffect> m_effect;
		uint64_t m_blending = C_MATERIAL_BLEND_DEFAULT;
		uint64_t m_state = C_MATERIAL_STATE_DEFAULT;
	};

	suniform			create_uniform(const char* name, uniform_type_t type);
	void				update_uniform(suniform& uniform, rttr::variant&& data);

} //- kokoro