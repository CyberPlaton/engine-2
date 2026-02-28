#pragma once
#include <engine/iservice.hpp>
#include <bgfx.h>

namespace kokoro
{
	inline static constexpr uint16_t C_BACKBUFFER_PASS_ID		= 0;
	inline static constexpr uint16_t C_GEOMETRY_PASS_ID			= 1;
	inline static constexpr uint16_t C_POSTPROCESS_PASS_ID		= C_GEOMETRY_PASS_ID + 1;
	inline static constexpr uint16_t C_MERGE_PASS_ID			= 254;
	inline static constexpr uint16_t C_IMGUI_PASS_ID			= 255;

	//- Right handed coordinate system means,
	//- X goes left-to-right,
	//- Y goes bottom-to-top,
	//- Z goes screen-to-eye (meaning the direction we are looking at is the negative Z-axis),
	//- for visual image check out https://github.com/bevyengine/bevy/discussions/10488
	inline static constexpr bx::Handedness::Enum C_HANDEDNESS	= bx::Handedness::Right;

	//------------------------------------------------------------------------------------------------------------------------
	class crender_service final : public iservice
	{
	public:
		crender_service() = default;
		~crender_service() = default;

		bool		init() override final;
		void		post_init() override final;
		void		shutdown() override final;
		void		update(float dt) override final;

		void		begin_frame();
		void		end_frame();
		void		submit_screen_quad(float scalex = 1.0f, float scaley = 1.0f);

	private:
		bgfx::FrameBufferHandle m_geometry_framebuffer	= BGFX_INVALID_HANDLE;
		bgfx::ProgramHandle m_merge_program				= BGFX_INVALID_HANDLE;
		bgfx::ProgramHandle m_apply_back0_program		= BGFX_INVALID_HANDLE;
		bgfx::ProgramHandle m_apply_back1_program		= BGFX_INVALID_HANDLE;
		bgfx::UniformHandle m_texture_chain_uniform		= BGFX_INVALID_HANDLE;
	};

} //- kokoro