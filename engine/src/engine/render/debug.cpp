#include <engine/render/debug.hpp>
#include <engine/math/vec2.hpp>
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
		constexpr std::array<uint16_t, 6> C_QUAD_INDICES = { 0, 1, 2, 0, 2, 3 };
		constexpr std::array<uint16_t, 12> C_QUAD_LINE_INDICES = { 0, 1, 1, 2, 2, 0, 0, 2, 2, 3, 3, 0 };
		constexpr std::array<std::string_view, static_cast<uint64_t>(cdebug_drawer::render_effect_count)> C_EFFECTS =
		{
			"engine/effects/debug/default.effect",
			"engine/effects/debug/default_textured.effect",
			"engine/effects/debug/default_textured.effect",
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

		for (auto i = 0; i < C_EFFECTS.size(); ++i)
		{
			m_effects[i] = rms.load<seffect>(C_EFFECTS[i]);
		}

		m_state.m_effect = m_effects[0];

		m_texture_sampler = create_uniform("s_texture", uniform_type_t::Sampler);
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::shutdown()
	{
		auto& rms = instance().service<cresource_manager_service>();
		rms.unload<seffect>(m_state.m_effect.id());

		bgfx::destroy(m_texture_sampler.m_handle);
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
		bgfx::discard(BGFX_DISCARD_ALL);
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
		if (i < m_effects.size())
		{
			m_state.m_effect = m_effects[i];
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
	cdebug_drawer& cdebug_drawer::triangle(const math::vec3_t& v0, const math::vec3_t& v1, const math::vec3_t& v2, uint32_t color /*= 0*/)
	{
		CPU_ZONE;

		set_texture({});

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

		set_texture({});

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
	cdebug_drawer& cdebug_drawer::texture(cview<stexture> texture_view, const math::vec3_t& v0, const math::vec3_t& v1, const math::vec3_t& v2,
		const math::vec3_t& v3, const math::vec4_t& source /*= { 0.0f, 0.0f, 1.0f, 1.0f }*/, uint32_t color /*= 0*/)
	{
		CPU_ZONE;

		//- In case the new texture is not valid, dont submit anything
		if (!texture_view)
		{
			return *this;
		}

		set_texture(texture_view);

		auto state = m_state.m_state;
		state &= ~BGFX_STATE_PT_MASK;
		set_state(state);

		const auto c = color == 0 ? m_state.m_color : color;
		math::vec2_t uv0 = { source.x, source.y }, uv1 = { source.x + source.z, source.y + source.w };

		if (!m_state.m_wireframe)
		{
			//- Set indices for quad
			const auto index_offset = vertex_count();
			auto& inds = indices();
			const auto index_count = inds.size();
			inds.resize(index_count + C_QUAD_INDICES.size());

			for (auto i = 0; i < C_QUAD_INDICES.size(); ++i)
			{
				inds[index_count + i] = index_offset + C_QUAD_INDICES[i];
			}
		}
		else
		{
			//- Set line topology for drawing the wireframe
			auto state = m_state.m_state;
			state &= ~BGFX_STATE_PT_MASK;
			state |= BGFX_STATE_PT_LINES | BGFX_STATE_LINEAA;
			if (m_state.m_state != state)
			{
				m_state.m_state = state;
			}

			//- Set indices for lines quad
			const auto index_offset = vertex_count();
			auto& inds = indices();
			const auto index_count = inds.size();
			inds.resize(index_count + C_QUAD_LINE_INDICES.size());

			for (auto i = 0; i < C_QUAD_LINE_INDICES.size(); ++i)
			{
				inds[index_count + i] = index_offset + C_QUAD_LINE_INDICES[i];
			}
		}

		//- Set vertices for quad
		auto& verts = vertices();
		verts.push_back(spos_color_uv_vertex{ v0.x, v0.y, v0.z, c, uv0.x, uv0.y });
		verts.push_back(spos_color_uv_vertex{ v1.x, v1.y, v1.z, c, uv1.x, uv0.y });
		verts.push_back(spos_color_uv_vertex{ v2.x, v2.y, v2.z, c, uv1.x, uv1.y });
		verts.push_back(spos_color_uv_vertex{ v3.x, v3.y, v3.z, c, uv0.x, uv1.y });
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
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::set_texture(cview<stexture> texture)
	{
		auto& curr = state();
		const auto new_handle = texture ? texture.get().m_handle.idx : bgfx::kInvalidHandle;
		const auto curr_handle = curr.m_texture ? curr.m_texture.get().m_handle.idx : bgfx::kInvalidHandle;

		if (new_handle != curr_handle)
		{
			flush();
			curr.m_texture = texture;
		}
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
		if (s.m_texture) { bgfx::setTexture(0, m_texture_sampler.m_handle, s.m_texture.get().m_handle); }
		bgfx::setState(s.m_state);
		bgfx::setTransform(C_MAT4_ID.value);
		if (s.m_effect) { bgfx::submit(s.m_view, s.m_effect.get().m_program); }

		//- Reset the geometry
		vertices().clear();
		indices().clear();
	}

} //- kokoro