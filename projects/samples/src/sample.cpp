#include <sample.hpp>
#include <engine/services/render_service.hpp>
#include <engine/services/window_service.hpp>

//------------------------------------------------------------------------------------------------------------------------
bool cgame::init()
{
	m_world = kokoro::world::create("samples");
	kokoro::world::promote(*m_world);
	return true;
}

//------------------------------------------------------------------------------------------------------------------------
void cgame::post_init()
{
	m_dd.init();
}

//------------------------------------------------------------------------------------------------------------------------
void cgame::shutdown()
{
	m_dd.shutdown();
	kokoro::world::destroy("samples");
	m_world = nullptr;
}

//------------------------------------------------------------------------------------------------------------------------
void cgame::update(float dt)
{
	kokoro::world::tick(*m_world, dt);

	const auto& rs = kokoro::instance().service<kokoro::crender_service>();
	const auto& ws = kokoro::instance().service<kokoro::cwindow_service>();

	m_dd.render_type(kokoro::cdebug_drawer::type_primitives)
		.begin(kokoro::C_GEOMETRY_PASS_ID, rs.geometry_framebuffer(),
			BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x882211ff)
		.view_rect(0, 0, ws.window_size().first, ws.window_size().second)
		.cull(kokoro::cdebug_drawer::culling_mode_ccw)
		.depth_test(kokoro::cdebug_drawer::depth_test_mode_none)
		.blend(kokoro::cdebug_drawer::blending_mode_none);

	m_dd.color(0x00ff22ff)
		.draw(
		{ 0.0f, 1.0f, 0.5f },
		{ 1.0f, -1.0f, 0.5f },
		{ -1.0f, -1.0f, 0.5f });

	m_dd.submit();
	m_dd.frame();
}
