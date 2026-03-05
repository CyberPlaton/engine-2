#pragma once
#include <engine/effect/effect.hpp>
#include <engine/render/texture.hpp>
#include <engine/iresource_manager_service.hpp>

namespace kokoro
{
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
			const stexture* m_texture = nullptr;
			uint64_t m_sampler_flags = 0;
			uint8_t m_sampler_stage = 0;
		};

		std::vector<ssampler_texture> m_sampler_textures;
		seffect* m_effect = nullptr;
		uint64_t m_blending = C_MATERIAL_BLEND_DEFAULT;
		uint64_t m_state = C_MATERIAL_STATE_DEFAULT;
	};

	//------------------------------------------------------------------------------------------------------------------------
	class cmaterial_resource_manager_service final : public iresource_manager_service<smaterial, smaterial_snapshot>
	{
	public:
		cmaterial_resource_manager_service() = default;
		~cmaterial_resource_manager_service() = default;

	protected:
		smaterial		do_instantiate(const smaterial_snapshot* snaps) override final;
		void			do_destroy(smaterial* inst) override final;
	};

} //- kokoro