#include <engine/services/render_service.hpp>
#include <bx.h>
#include <engine/services/window_service.hpp>
#include <engine/effect/effect_parser.hpp>
#include <engine/effect/effect.hpp>
#include <engine/material/material.hpp>
#include <engine/effect/embedded_shaders.hpp>
#include <engine/effect/shader.hpp>
#include <engine/render/vertices.hpp>
#include <engine/math/mat4.hpp>
#include <engine.hpp>

namespace kokoro
{
	namespace
	{
		uint16_t width = -1, height = -1;
		auto C_MAT4_ID = math::mat4_t(1.0f);

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	bool crender_service::init()
	{
		auto window = instance().service<cwindow_service>().main_window();

		const uint64_t reset = 0
			| BGFX_RESET_MSAA_X4
			| BGFX_RESET_MAXANISOTROPY
			| BGFX_RESET_VSYNC;

		bgfx::Init data;
		data.vendorId = BGFX_PCI_ID_NONE;
		data.resolution.width = window.m_width;
		data.resolution.height = window.m_height;
		data.resolution.reset = reset;
		data.type = bgfx::RendererType::Direct3D11;
		data.platformData.nwh = native_window_handle(window.m_window);
		data.platformData.ndt = nullptr;

		if (!bgfx::init(data))
		{
			return false;
		}

		width = window.m_width;
		height = window.m_height;
		bx::mtxIdentity(C_MAT4_ID.value);

		//- Create programs required for main loop
		{
			static const filepath_t C_MERGE_EFFECT_FILEPATH = "engine/effects/merge.effect";
			static const filepath_t C_APPLY0_EFFECT_FILEPATH = "engine/effects/apply_back0.effect";
			static const filepath_t C_APPLY1_EFFECT_FILEPATH = "engine/effects/apply_back1.effect";

			auto& rms = instance().service<cresource_manager_service>();
			m_merge_program = rms.load<seffect>(C_MERGE_EFFECT_FILEPATH);
			m_apply_back0_program = rms.load<seffect>(C_APPLY0_EFFECT_FILEPATH);
			m_apply_back1_program = rms.load<seffect>(C_APPLY1_EFFECT_FILEPATH);
		}

		//- Create default render target with color and depth
		{
			auto depth = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT);
			auto color = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
				BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT |
				BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT |
				BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

			bgfx::TextureHandle attachments[] = { color, depth };
			m_geometry_framebuffer = bgfx::createFrameBuffer(BX_COUNTOF(attachments), attachments, true);
		}

		//- Create default uniforms
		{
			m_texture_chain_uniform = bgfx::createUniform("s_texture", uniform_type_t::Sampler);
		}

		bgfx::setViewName(C_BACKBUFFER_PASS_ID, "Backbuffer");
		bgfx::setViewName(C_GEOMETRY_PASS_ID, "Geometry");
		bgfx::setViewName(C_POSTPROCESS_PASS_ID, "Postprocess 0");
		for (auto i = C_POSTPROCESS_PASS_ID + 1; i < C_MERGE_PASS_ID; ++i)
		{
			bgfx::setViewName(i, fmt::format("Postprocess {}", i - 1).c_str());
		}
		bgfx::setViewName(C_MERGE_PASS_ID, "Postprocess Combine");
		bgfx::setViewName(C_IMGUI_PASS_ID, "ImGui");

		bgfx::setDebug(BGFX_DEBUG_STATS);
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::post_init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::shutdown()
	{
		auto& rms = instance().service<cresource_manager_service>();
		rms.unload<seffect>(m_merge_program.id());
		rms.unload<seffect>(m_apply_back0_program.id());
		rms.unload<seffect>(m_apply_back1_program.id());
		bgfx::destroy(m_geometry_framebuffer);
		bgfx::destroy(m_texture_chain_uniform);
		bgfx::shutdown();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::update(float dt)
	{
		auto window = instance().service<cwindow_service>().main_window();

		if (window.m_width != width || window.m_height != height)
		{
			bgfx::reset(window.m_width, window.m_height);
			width = window.m_width;
			height = window.m_height;
		}
	}

	//- Start a new rendering frame. This will set everything up for geometry drawing to our default framebuffer on which
	//- we can operate later and draw as final image to screen.
	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::begin_frame()
	{
		bgfx::touch(C_GEOMETRY_PASS_ID);
		bgfx::setViewFrameBuffer(C_GEOMETRY_PASS_ID, m_geometry_framebuffer);
		bgfx::setViewClear(C_GEOMETRY_PASS_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0);
		bgfx::setViewTransform(C_GEOMETRY_PASS_ID, C_MAT4_ID.value, C_MAT4_ID.value);
		bgfx::setViewRect(C_GEOMETRY_PASS_ID, 0, 0, width, height);

		//- Begin ImGui if required
	}

	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::end_frame()
	{
		bgfx::setViewFrameBuffer(C_BACKBUFFER_PASS_ID, BGFX_INVALID_HANDLE);
		bgfx::setViewClear(C_BACKBUFFER_PASS_ID, BGFX_CLEAR_COLOR, 0x000055FF);
		bgfx::setViewTransform(C_BACKBUFFER_PASS_ID, C_MAT4_ID.value, C_MAT4_ID.value);
		bgfx::setViewRect(C_BACKBUFFER_PASS_ID, 0, 0, width, height);
		bgfx::setState(0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_MSAA);

		//- Set the accumulated framebuffer texture for rendering to screen
		const auto handle = bgfx::getTexture(m_geometry_framebuffer);
		bgfx::setTexture(0, m_texture_chain_uniform, handle);

		submit_screen_quad();

		if (m_merge_program)
		{
			bgfx::submit(C_BACKBUFFER_PASS_ID, m_merge_program.get().m_program);
		}

		//- End ImGui if required

		//- Kicking render thread to process submitted geometry
		bgfx::frame();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::submit_screen_quad(float scalex /*= 1.0f*/, float scaley /*= 1.0f*/)
	{
		const auto& layout = spos_uv_vertex::get().layout();

		if (3 == bgfx::getAvailTransientVertexBuffer(3, layout))
		{
			bgfx::TransientVertexBuffer vb;
			bgfx::allocTransientVertexBuffer(&vb, 3, layout);
			auto* vertex = (spos_uv_vertex*)vb.data;

			const auto _originBottomLeft = bgfx::getCaps()->originBottomLeft;
			const float zz = 0.0f;

			const float minx = -scalex;
			const float maxx = scalex;
			const float miny = -scaley;
			const float maxy = scaley;

			const float minu = -1.0f;
			const float maxu = 1.0f;

			float minv = 0.0f;
			float maxv = 2.0f;

			if (_originBottomLeft)
			{
				std::swap(minv, maxv);
				minv -= 1.0f;
				maxv -= 1.0f;
			}

			vertex[0].m_x = -1.0f;
			vertex[0].m_y = -1.0f;
			vertex[0].m_z = 0.0f;
			vertex[0].m_u = 0.0f;
			vertex[0].m_v = 0.0f;

			vertex[1].m_x = 3.0f;
			vertex[1].m_y = -1.0f;
			vertex[1].m_z = 0.0f;
			vertex[1].m_u = 2.0f;
			vertex[1].m_v = 0.0f;

			vertex[2].m_x = -1.0f;
			vertex[2].m_y = 3.0f;
			vertex[2].m_z = 0.0f;
			vertex[2].m_u = 0.0f;
			vertex[2].m_v = 2.0f;

			bgfx::setVertexBuffer(0, &vb);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::bind_builtin_uniforms()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	auto crender_service::merge_program(uint16_t postprocesses) const -> const cview<seffect>
	{
		return (postprocesses % 2 == 0) ? m_apply_back0_program :
			m_apply_back1_program;
	}

} //- kokoro