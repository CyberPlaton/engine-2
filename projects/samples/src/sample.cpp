#include <sample.hpp>
#include <engine/services/render_service.hpp>
#include <engine/services/window_service.hpp>
#include <engine/services/world_service.hpp>
#include <engine/components/postprocess_volume.hpp>
#include <engine/effect/embedded_shaders.hpp>
#include <engine.hpp>
#include <fmt.h>

//------------------------------------------------------------------------------------------------------------------------
bool cgame::init()
{
	return true;
}

//------------------------------------------------------------------------------------------------------------------------
void cgame::post_init()
{
	kokoro::sworld_snapshot world;
	world.m_name = "sample";
	world.m_cfg.m_threads = 0;
	world.m_cfg.m_modules = { "srender_module" };

	std::array<std::string_view, 11> postprocess_names =
	{
		"bloom",
		"blur",
		"chromatic_aberration",
		"filmgrain",
		"grayscale",
		"invert",
		"posterize",
		"scanlines",
		"sepia",
		"sharpen",
		"vignette"
	};

	for (auto i = 0; i < 11; ++i)
	{
		const auto name = fmt::format("post process #{}", i);

		auto& e = world.m_scene.m_entities.emplace_back();
		e.m_name = name;

		{
			kokoro::spostprocess_snapshot postprocess;
			postprocess.m_name = name;

			auto& subpass = postprocess.m_passes.emplace_back();
			subpass.m_effect = fmt::format("engine/postprocesses/{}.effect", postprocess_names[i]);
			subpass.m_blending = kokoro::spostprocess_subpass::C_BLEND_DEFAULT;
			subpass.m_state = kokoro::spostprocess_subpass::C_STATE_DEFAULT;
			subpass.m_backbuffer_ratio = kokoro::backbuffer_ratio_t::Equal;

			kokoro::world::component::spostprocess_volume comp;
			comp.m_postprocess = kokoro::instance().service<kokoro::cresource_manager_service>().load_from_snapshot<kokoro::spostprocess>(postprocess);

			auto& c = e.m_comps.emplace_back();
			c.m_type_name = rttr::type::get<kokoro::world::component::spostprocess_volume>().get_name().data();
			c.m_data = comp;
		}
	}

	m_world = kokoro::instance().service<kokoro::cresource_manager_service>().load_from_snapshot<kokoro::cworld>(world);
	kokoro::instance().service<kokoro::cworld_service>().promote(m_world);
	m_dd.init();
}

//------------------------------------------------------------------------------------------------------------------------
void cgame::shutdown()
{
	m_dd.shutdown();
	kokoro::instance().service<kokoro::cresource_manager_service>().unload<kokoro::cworld>(m_world.id());
}

//------------------------------------------------------------------------------------------------------------------------
void cgame::update(float dt)
{
	auto active = kokoro::instance().service<kokoro::cworld_service>().active();
	if (!active)
	{
		return;
	}

	active.get().tick(dt);

	const auto& rs = kokoro::instance().service<kokoro::crender_service>();
	const auto& ws = kokoro::instance().service<kokoro::cwindow_service>();

	m_dd.begin(kokoro::C_GEOMETRY_PASS_ID, rs.geometry_framebuffer(),
			BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x882211ff)
		.view_rect(0, 0, ws.window_size().first, ws.window_size().second)
		.cull(kokoro::cdebug_drawer::culling_mode_ccw)
		.depth_test(kokoro::cdebug_drawer::depth_test_mode_none)
		.blend(kokoro::cdebug_drawer::blending_mode_alpha)
		.wireframe(true);

	const auto unproject = [](const kokoro::math::mat4_t& mtx, const kokoro::math::vec4_t& p)
		{
			using namespace kokoro;

			// 2. Multiply by the Inverse View-Projection Matrix
			// This moves the point from "Screen space" back towards "World space"
			math::vec4_t world_point = mtx.inverse() * p;

			// 3. The Perspective Divide (CRITICAL STEP)
			// After multiplying by an inverse projection, the w component 
			// represents the scaling factor caused by the original perspective.
			// We must divide by w to get the actual x, y, z coordinates.
			if (world_point.w != 0.0f)
			{
				world_point.x /= world_point.w;
				world_point.y /= world_point.w;
				world_point.z /= world_point.w;
			}

			return math::vec3_t(world_point.x, world_point.y, world_point.z);
		};

	// 1. Get Delta Time (dt) and an accumulator for rotation
	static float accumulator = 0.0f;
	accumulator += dt * 0.01f;

	kokoro::math::mat4_t mtx(1.0f);

	// 2. Create the components of the Model Matrix
	// We'll scale it down because 1.0 is the edge of the screen.
	float scaleFactor = 0.25f;
	
	mtx.scale(kokoro::math::vec3_t(scaleFactor))
		.rotate(kokoro::math::vec3_t(0.0f, 0.0f, accumulator))
		.translate(kokoro::math::vec3_t(0.0f, 0.0f, 0.0f));

	//- Drawing triangle shapes
	{
		kokoro::math::vec3_t v0 = { 0.0f, 1.0f, 0.0f };
		kokoro::math::vec3_t v1 = { 1.0f, -1.0f, 0.0f };
		kokoro::math::vec3_t v2 = { -1.0f, -1.0f, 0.0f };

		kokoro::math::vec3_t v3 = { 0.0f, 1.5f, 0.0f };
		kokoro::math::vec3_t v4 = { 1.5f, -1.25f, 0.0f };
		kokoro::math::vec3_t v5 = { -1.5f, -0.75f, 0.0f };

		m_dd.wireframe(false)
			.triangle(
				mtx * v0,
				mtx * v1,
				mtx * v2,
				0xff22aaee)
			.wireframe(true)
			.triangle(
				mtx * v3,
				mtx * v4,
				mtx * v5,
				0xff11ff00)
			.submit();
	}

	//- Drawing quads
	{
		kokoro::math::vec3_t v0 = { 0.0f,  1.0f, 0.0f }; // Top
		kokoro::math::vec3_t v1 = { 1.0f,  0.0f, 0.0f }; // Right
		kokoro::math::vec3_t v2 = { 0.0f, -1.0f, 0.0f }; // Bottom
		kokoro::math::vec3_t v3 = { -1.0f,  0.0f, 0.0f }; // Left

		m_dd.quad(mtx * v0, mtx * v1, mtx * v2, mtx * v3, 0xff552211)
			.submit();
	}

	//- Drawing lines
	{
		kokoro::math::vec3_t v0 = { 0.0f,  0.0f, 0.0f }; // Top
		kokoro::math::vec3_t v1 = { 100.0f,  50.0f, 0.0f }; // Right
		m_dd.line(v0, v1, 0xff0000ff)
			.submit();
	}
	m_dd.frame();
}
