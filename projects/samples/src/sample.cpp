#include <sample.hpp>
#include <engine/services/render_service.hpp>

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

	m_dd.begin(kokoro::C_GEOMETRY_PASS_ID);

	m_dd.draw(
		{ 0.0f, 1.0f, 0.0f },
		{ 1.0f, -1.0f, 0.0f },
		{ -1.0f, -1.0f, 0.0f });

	m_dd.submit();
	m_dd.frame();
}
