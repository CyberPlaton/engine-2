#include <engine/render/debug.hpp>
#include <core/profile.hpp>

namespace kokoro
{
	namespace
	{
		constexpr uint64_t C_VERTICES_RESERVE_COUNT = 50000;
		constexpr uint64_t C_INDICES_RESERVE_COUNT = C_VERTICES_RESERVE_COUNT * 6;
		constexpr auto C_MAT4_ID = math::mat4_t(1.0f);
		constexpr std::array<uint16_t, 6> C_TRIANGLE_LINE_INDICES = { 0, 1, 1, 2, 2, 0};
		constexpr std::array<uint16_t, 3> C_TRIANGLE_INDICES = { 0, 1, 2 };
		constexpr std::array<std::string_view, 1> C_EFFECTS =
		{
			"engine/effects/debug/default.effect",
		};

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer::cdebug_drawer()
	{
		m_vertices.reserve(C_VERTICES_RESERVE_COUNT);
		m_indices.reserve(C_INDICES_RESERVE_COUNT);
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cdebug_drawer::init()
	{
		auto& rms = instance().service<cresource_manager_service>();
		m_state.m_effect = rms.load<seffect>(C_EFFECTS[0]);

		//- Setup for render types
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::shutdown()
	{
		auto& rms = instance().service<cresource_manager_service>();
		rms.unload<seffect>(m_state.m_effect.path());
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::begin(bgfx::ViewId view, bgfx::FrameBufferHandle fbh, uint16_t flags /*= 0*/, uint32_t color /*= 0*/,
		float depth /*= 1.0f*/, uint8_t stencil /*= 0*/)
	{
		CPU_ZONE;

		auto& s = state();
		s.m_state = C_STATE_DEFAULT;	//- Reset to default state and avoid leakage, we expect state settings on a per-frame basis anyway
		s.m_view = view;
		s.m_fbh = fbh;
		s.m_clear_flags = flags;
		s.m_clear_color = color;
		s.m_clear_depth = depth;
		s.m_clear_stencil = stencil;
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::frame()
	{
		CPU_ZONE;
		submit();
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::view_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
	{
		auto& s = state();
		s.m_view_x = x;
		s.m_view_y = y;
		s.m_view_w = w;
		s.m_view_h = h;
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::depth_test(depth_test_mode mode)
	{
		//- Erase previously set depth test mode and set the new one
		state().m_state &= ~BGFX_STATE_DEPTH_TEST_MASK;
		switch (mode)
		{
		case depth_test_mode_none:
		default:
			break;
		case depth_test_mode_less:
			state().m_state |= BGFX_STATE_DEPTH_TEST_LESS;
			break;
		case depth_test_mode_less_equal:
			state().m_state |= BGFX_STATE_DEPTH_TEST_LEQUAL;
			break;
		case depth_test_mode_equal:
			state().m_state |= BGFX_STATE_DEPTH_TEST_EQUAL;
			break;
		case depth_test_mode_greater_equal:
			state().m_state |= BGFX_STATE_DEPTH_TEST_GEQUAL;
			break;
		case depth_test_mode_greater:
			state().m_state |= BGFX_STATE_DEPTH_TEST_GREATER;
			break;
		case depth_test_mode_not_equal:
			state().m_state |= BGFX_STATE_DEPTH_TEST_NOTEQUAL;
			break;
		case depth_test_mode_never:
			state().m_state |= BGFX_STATE_DEPTH_TEST_NEVER;
			break;
		case depth_test_mode_always:
			state().m_state |= BGFX_STATE_DEPTH_TEST_ALWAYS;
			break;
		}
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::cull(culling_mode mode)
	{
		//- Erase previously set culling mode and set the new one
		state().m_state &= ~BGFX_STATE_CULL_MASK;
		switch (mode)
		{
		case culling_mode_none:
		default:
			break;
		case culling_mode_cw:
			state().m_state |= BGFX_STATE_CULL_CW;
			break;
		case culling_mode_ccw:
			state().m_state |= BGFX_STATE_CULL_CCW;
			break;
		}
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::blend(blending_mode mode)
	{
		//- Erase previously set blending mode and set the new one
		state().m_state &= ~BGFX_STATE_BLEND_MASK;
		switch (mode)
		{
		case blending_mode_none:
		default:
			break;
		case blending_mode_alpha:
			//- Source * Alpha + Dest * (1 - Alpha)
			state().m_state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
			break;
		case blending_mode_additive:
			//- Source + Dest
			state().m_state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE);
			break;
		case blending_mode_multiplicative:
			//- Source * Dest
			state().m_state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_DST_COLOR, BGFX_STATE_BLEND_ZERO);
			break;
		case blending_mode_premultiplied:
			//- Source + Dest * (1 - Alpha)
			state().m_state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
			break;
		case blending_mode_screen:
			//- 1 - ((1 - Source) * (1 - Dest))
			state().m_state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_COLOR);
			break;
		case blending_mode_darken:
			//- min(Source, Dest)
			state().m_state |=
				BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_MIN) |
				BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE);
			break;
		case blending_mode_lighten:
			//- max(Source, Dest)
			state().m_state |=
				BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_MAX) |
				BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE);
			break;
		}
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::effect(render_effect value)
	{
		const auto i = static_cast<uint64_t>(value);
		if (i < C_EFFECTS.size())
		{
			auto& rms = instance().service<cresource_manager_service>();
			state().m_effect = rms.load<seffect>(C_EFFECTS[i]);
		}
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::color(uint32_t value)
	{
		state().m_color = value;
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::transform(const math::mat4_t& mtx)
	{
		state().m_matrix = mtx;
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::translate(const math::vec3_t& value)
	{
		state().m_matrix.translate(value);
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::scale(const math::vec3_t& value)
	{
		state().m_matrix.scale(value);
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::rotate(const math::vec3_t& value)
	{
		state().m_matrix.rotate(value);
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::wireframe(const bool value)
	{
		state().m_wireframe = value;
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::triangle(const math::vec3_t& v0, const math::vec3_t& v1, const math::vec3_t& v2,
		uint32_t color /*= 0*/)
	{
		CPU_ZONE;

		const auto c = color == 0 ? m_state.m_color : color;

		if (!m_state.m_wireframe)
		{
			//- Remove any topology state and handle as default triangle list
			auto state = m_state.m_state;
			state &= ~BGFX_STATE_PT_MASK;
			set_state(state);

			//- Set indices for triangle
			const auto index_offset = vertex_count();
			auto& inds = indices();
			const auto index_count = inds.size();
			inds.resize(index_count + C_TRIANGLE_INDICES.size());

			for (auto i = 0; i < C_TRIANGLE_INDICES.size(); ++i)
			{
				inds[index_count + i] = index_offset + C_TRIANGLE_INDICES[i];
			}
		}
		else
		{
			//- Set line topology for drawing the wireframe
			auto state = m_state.m_state;
			state &= ~BGFX_STATE_PT_MASK;
			state |= BGFX_STATE_PT_LINES | BGFX_STATE_LINEAA;
			set_state(state);

			//- Set indices for lines triangle
			const auto index_offset = vertex_count();
			auto& inds = indices();
			const auto index_count = inds.size();
			inds.resize(index_count + C_TRIANGLE_LINE_INDICES.size());

			for (auto i = 0; i < C_TRIANGLE_LINE_INDICES.size(); ++i)
			{
				inds[index_count + i] = index_offset + C_TRIANGLE_LINE_INDICES[i];
			}
		}

		//- Set vertices for triangle
		auto& verts = vertices();
		verts.push_back(spos_color_uv_vertex{ v0.x, v0.y, v0.z, c, 0.0f, 1.0f });
		verts.push_back(spos_color_uv_vertex{ v1.x, v1.y, v1.z, c, 0.0f, 1.0f });
		verts.push_back(spos_color_uv_vertex{ v2.x, v2.y, v2.z, c, 0.0f, 1.0f });
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::quad(const math::vec3_t& v0, const math::vec3_t& v1, const math::vec3_t& v2, const math::vec3_t& v3,
		uint32_t color /*= 0*/)
	{
		triangle(v0, v1, v2, color);
		triangle(v0, v2, v3, color);
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::line(const math::vec3_t& v0, const math::vec3_t& v1, uint32_t color /*= 0*/)
	{
		CPU_ZONE;

		auto state = m_state.m_state;
		state &= ~BGFX_STATE_PT_MASK;
		state |= BGFX_STATE_PT_LINES | BGFX_STATE_LINEAA;
		set_state(state);

		const auto c = color == 0 ? m_state.m_color : color;

		auto& verts = vertices();
		verts.push_back(spos_color_uv_vertex{ v0.x, v0.y, v0.z, c, 0.0f, 1.0f });
		verts.push_back(spos_color_uv_vertex{ v1.x, v1.y, v1.z, c, 0.0f, 1.0f });
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer::srender_state& cdebug_drawer::state()
	{
		return m_state;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::set_state(uint64_t value)
	{
		auto& curr = state();

		if (value != 0 && value != curr.m_state)
		{
			flush();
			curr.m_state = value;
		}

		bgfx::setState(curr.m_state);
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cdebug_drawer::indices() -> std::vector<uint16_t>&
	{
		return m_indices;
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cdebug_drawer::vertices() -> std::vector<spos_color_uv_vertex>&
	{
		return m_vertices;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void* cdebug_drawer::vertex_data() const
	{
		return reinterpret_cast<void*>(const_cast<spos_color_uv_vertex*>(m_vertices.data()));
	}

	//------------------------------------------------------------------------------------------------------------------------
	uint32_t cdebug_drawer::vertex_count() const
	{
		return static_cast<uint32_t>(m_vertices.size());
	}

	//------------------------------------------------------------------------------------------------------------------------
	const bgfx::VertexLayout& cdebug_drawer::vertex_layout() const
	{
		return spos_color_uv_vertex::get().layout();
	}

	//------------------------------------------------------------------------------------------------------------------------
	bgfx::VertexLayoutHandle cdebug_drawer::vertex_handle() const
	{
		return spos_color_uv_vertex::get().handle();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::submit_buffers()
	{
		CPU_ZONE;

		const auto& layout = vertex_layout();
		if (const auto count = vertex_count(); count > 0 && count == bgfx::getAvailTransientVertexBuffer(count, layout))
		{
			bgfx::TransientVertexBuffer tvb;
			bgfx::allocTransientVertexBuffer(&tvb, count, layout);
			std::memcpy(tvb.data, vertex_data(), static_cast<uint64_t>(count * layout.m_stride));
			bgfx::setVertexBuffer(0, &tvb);

			const auto& inds = indices();
			if (!inds.empty())
			{
				const auto indices_count = static_cast<uint32_t>(inds.size());
				bgfx::TransientIndexBuffer tib;
				bgfx::allocTransientIndexBuffer(&tib, indices_count);
				std::memcpy(tib.data, inds.data(), indices_count * sizeof(uint16_t));
				bgfx::setIndexBuffer(&tib);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::flush()
	{
		CPU_ZONE;

		//- Set geometry and indices for drawing call
		submit_buffers();

		//- Set the render state for drawing call
		auto& s = state();

		bgfx::setViewFrameBuffer(s.m_view, s.m_fbh);
		bgfx::setViewClear(s.m_view, s.m_clear_flags, s.m_clear_color);
		bgfx::setViewRect(s.m_view, s.m_view_x, s.m_view_y, s.m_view_w, s.m_view_h);
		bgfx::setViewTransform(s.m_view, C_MAT4_ID.value, C_MAT4_ID.value);

		set_state(s.m_state);

		bgfx::setTransform(C_MAT4_ID.value);

		bgfx::submit(s.m_view, s.m_effect.get().m_program);

		//- Reset the geometry
		vertices().clear();
		indices().clear();
	}

} //- kokoro