#include <engine/services/render_service.hpp>
#include <engine/services/window_service.hpp>
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
		bgfx::setViewFrameBuffer(C_GEOMETRY_PASS_ID, geometry_framebuffer());
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
		bgfx::setTexture(0, uniform, bgfx::getTexture(geometry_framebuffer()));

		submit_screen_quad();

		bgfx::submit(C_BACKBUFFER_PASS_ID, program);

		//- End ImGui if required

		//- Kicking render thread to process submitted geometry
		bgfx::frame();
	}

} //- kokoro