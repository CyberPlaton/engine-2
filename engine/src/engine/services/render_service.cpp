#include <engine/services/render_service.hpp>
#include <engine/services/window_service.hpp>
#include <engine/effect/effect_parser.hpp>
#include <engine/effect/effect.hpp>
#include <engine/effect/embedded_shaders.hpp>
#include <engine/effect/shader.hpp>
#include <engine.hpp>

namespace kokoro
{
	namespace
	{
		uint16_t width = -1, height = -1;

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

		//- Create programs required for main loop
		{
			core::memory_ref_t default_vs;
			{
				ceffect_parser parser(sembedded_shaders::get(sembedded_shaders::vs_postprocess));
				const auto output = parser.parse();

				scompile_options options;
				options.m_type = scompile_options::shader_type_vertex;
				options.m_name = "vs_postprocess";
				default_vs = compile_shader_from_string(output.m_vs.c_str(), options);

				if (!default_vs)
				{
					return false;
				}
			}

			const auto do_create_program = [&](auto name)
				{
					ceffect_parser parser(sembedded_shaders::get(name));
					const auto output = parser.parse();

					scompile_options options;
					options.m_type = scompile_options::shader_type_pixel;
					options.m_name = name;

					auto mem = compile_shader_from_string(output.m_ps.c_str(), options);
					return create_program(default_vs, mem);
				};

			m_merge_program = do_create_program("ps_combine");
			m_apply_back0_program = do_create_program("ps_postprocess_apply_back0");
			m_apply_back1_program = do_create_program("ps_postprocess_apply_back1");
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
			m_texture_chain_uniform = bgfx::createUniform("s_texture", suniform::type::Sampler);
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::post_init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::shutdown()
	{
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
		bgfx::setViewClear(C_GEOMETRY_PASS_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x555555FF);
		bgfx::setViewTransform(C_GEOMETRY_PASS_ID, mat4id, mat4id);
		bgfx::setViewRect(C_GEOMETRY_PASS_ID, 0, 0, width, height);

		//- Begin ImGui if required
	}

	//------------------------------------------------------------------------------------------------------------------------
	void crender_service::end_frame()
	{
		bgfx::setViewFrameBuffer(C_BACKBUFFER_PASS_ID, BGFX_INVALID_HANDLE);
		bgfx::setViewClear(C_BACKBUFFER_PASS_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030FF);
		bgfx::setViewTransform(C_BACKBUFFER_PASS_ID, mat4id, mat4id);
		bgfx::setViewRect(C_BACKBUFFER_PASS_ID, 0, 0, width, height);

		//- Set the accumulated framebuffer texture for rendering to screen
		bgfx::setTexture(0, m_texture_chain_uniform, bgfx::getTexture(m_geometry_framebuffer));

		submit_screen_quad();

		bgfx::submit(C_BACKBUFFER_PASS_ID, m_merge_program);

		//- End ImGui if required

		//- Kicking render thread to process submitted geometry
		bgfx::frame();
	}

} //- kokoro