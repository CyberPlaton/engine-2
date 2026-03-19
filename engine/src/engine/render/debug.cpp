#include <engine/render/debug.hpp>
#include <core/profile.hpp>

namespace kokoro
{
	namespace
	{
		constexpr auto C_MAT4_ID = math::mat4_t(1.0f);
		constexpr std::array<std::string_view, 3> C_EFFECTS =
		{
			"engine/effects/debug/default.effect",
			"engine/effects/debug/default.effect",
			"engine/effects/debug/default.effect"
		};

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	bool cdebug_drawer::init()
	{
		auto& erm = instance().service<ceffect_resource_manager_service>();
		m_shape_data.m_state.m_effect = erm.instantiate(C_EFFECTS[0]);
		m_sprite_data.m_state.m_effect = erm.instantiate(C_EFFECTS[1]);
		m_text_data.m_state.m_effect = erm.instantiate(C_EFFECTS[2]);

		//- Setup for render types
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::shutdown()
	{
		auto& erm = instance().service<ceffect_resource_manager_service>();
		erm.destroy(m_shape_data.m_state.m_effect);
		erm.destroy(m_sprite_data.m_state.m_effect);
		erm.destroy(m_text_data.m_state.m_effect);
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::begin(bgfx::ViewId view, bgfx::FrameBufferHandle fbh, uint16_t flags /*= 0*/, uint32_t color /*= 0*/,
		float depth /*= 1.0f*/, uint8_t stencil /*= 0*/)
	{
		CPU_ZONE;

		auto& s = state();
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

		//- 
		for (auto i = 0; i < (int)type_count; ++i)
		{
			render_type((type)i).submit();
		}
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
	cdebug_drawer& cdebug_drawer::render_type(type value)
	{
		m_current_type = value == type_count ? type_primitives : value;
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
			auto& erm = instance().service<ceffect_resource_manager_service>();
			state().m_effect = erm.instantiate(C_EFFECTS[i]);
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

		//- Set state for drawing either filled triangles or lines
		auto new_state = state().m_state;
		new_state &= ~BGFX_STATE_PT_MASK;
		if (state().m_wireframe)
		{
			new_state |= BGFX_STATE_PT_LINES | BGFX_STATE_LINEAA;
		}
		set_state(new_state);

		const auto index_offset = vertex_count();
		if (state().m_wireframe)
		{
			//- Push required indices to correctly close the triangle
			push_index(index_offset + 0);
			push_index(index_offset + 1);
			push_index(index_offset + 1);
			push_index(index_offset + 2);
			push_index(index_offset + 2);
			push_index(index_offset + 0);
		}
		else
		{
			push_index(index_offset + 0);
			push_index(index_offset + 1);
			push_index(index_offset + 2);
		}

		const auto c = color == 0 ? state().m_color : color;
		push_vertex(v0.x, v0.y, v0.z, c);
		push_vertex(v1.x, v1.y, v1.z, c);
		push_vertex(v2.x, v2.y, v2.z, c);
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

		auto new_state = state().m_state;
		new_state &= ~BGFX_STATE_PT_MASK;
		new_state |= BGFX_STATE_PT_LINES;
		set_state(new_state);

		const auto c = color == 0 ? state().m_color : color;
		push_vertex(v0.x, v0.y, v0.z, c);
		push_vertex(v1.x, v1.y, v1.z, c);
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer::srender_state& cdebug_drawer::state()
	{
		switch (m_current_type)
		{
		default:
		case type_primitives:
			return m_shape_data.m_state;
		case type_sprite:
			return m_sprite_data.m_state;
		case type_text:
			return m_text_data.m_state;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::set_state(uint64_t new_state /*= 0*/)
	{
		auto& curr = state();

		if (new_state != 0 &&
			new_state != curr.m_state)
		{
			flush();
			curr.m_state = new_state;
		}

		bgfx::setState(curr.m_state);
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cdebug_drawer::indices_vector() -> std::vector<uint16_t>&
	{
		switch (m_current_type)
		{
		default:
		case type_primitives:
			return m_shape_data.m_indices;
		case type_sprite:
			return m_sprite_data.m_indices;
		case type_text:
			return m_text_data.m_indices;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::vertex_clear()
	{
		switch (m_current_type)
		{
		default:
		case type_primitives:
			m_shape_data.m_vertices.clear();
			break;
		case type_sprite:
			m_sprite_data.m_vertices.clear();
			break;
		case type_text:
			m_text_data.m_vertices.clear();
			break;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void* cdebug_drawer::vertex_data() const
	{
		switch (m_current_type)
		{
		default:
		case type_primitives:
			return (void*)m_shape_data.m_vertices.data();
		case type_sprite:
			return (void*)m_sprite_data.m_vertices.data();
		case type_text:
			return (void*)m_text_data.m_vertices.data();
		}
		return nullptr;
	}

	//------------------------------------------------------------------------------------------------------------------------
	uint32_t cdebug_drawer::vertex_count() const
	{
		switch (m_current_type)
		{
		default:
		case type_primitives:
			return static_cast<uint32_t>(m_shape_data.m_vertices.size());
		case type_sprite:
			return static_cast<uint32_t>(m_sprite_data.m_vertices.size());
		case type_text:
			return static_cast<uint32_t>(m_text_data.m_vertices.size());
		}
		return 0;
	}

	//------------------------------------------------------------------------------------------------------------------------
	const bgfx::VertexLayout& cdebug_drawer::vertex_layout() const
	{
		switch (m_current_type)
		{
		default:
		case type_primitives:
			return spos_color_vertex::get().layout();
		case type_sprite:
			return spos_color_uv_vertex::get().layout();
		case type_text:
			return spos_color_uv_vertex::get().layout();
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	bgfx::VertexLayoutHandle cdebug_drawer::vertex_handle() const
	{
		switch (m_current_type)
		{
		default:
		case type_primitives:
			return spos_color_vertex::get().handle();
		case type_sprite:
			return spos_color_uv_vertex::get().handle();
		case type_text:
			return spos_color_uv_vertex::get().handle();
		}
		return BGFX_INVALID_HANDLE;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cdebug_drawer::submit_buffers()
	{
		const auto count = vertex_count();
		if (count > 0)
		{
			const auto& layout = vertex_layout();

			if (count == bgfx::getAvailTransientVertexBuffer(count, layout))
			{
				bgfx::TransientVertexBuffer tvb;
				bgfx::allocTransientVertexBuffer(&tvb, count, layout);
				bx::memCopy(tvb.data, vertex_data(), static_cast<uint64_t>(count * layout.m_stride));
				bgfx::setVertexBuffer(0, &tvb);

				const auto& indices = indices_vector();
				if (!indices.empty())
				{
					const auto indices_count = static_cast<uint32_t>(indices.size());
					bgfx::TransientIndexBuffer tib;
					bgfx::allocTransientIndexBuffer(&tib, indices_count);
					bx::memCopy(tib.data, indices.data(), indices_count * sizeof(uint16_t));
					bgfx::setIndexBuffer(&tib);
				}
			}
			return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::flush()
	{
		if (submit_buffers())
		{
			const auto& s = state();

			bgfx::setViewFrameBuffer(s.m_view, s.m_fbh);
			bgfx::setViewClear(s.m_view, s.m_clear_flags, s.m_clear_color);
			bgfx::setViewRect(s.m_view, s.m_view_x, s.m_view_y, s.m_view_w, s.m_view_h);
			bgfx::setViewTransform(s.m_view, C_MAT4_ID.value, C_MAT4_ID.value);

			set_state();

			bgfx::setTransform(C_MAT4_ID.value);

			const auto* effect = instance().service<ceffect_resource_manager_service>().get(state().m_effect);
			bgfx::submit(state().m_view, effect->m_program);
		}

		vertex_clear();
		indices_vector().clear();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cdebug_drawer::push_index(uint16_t i)
	{
		switch (m_current_type)
		{
		default:
		case type_primitives:
			m_shape_data.m_indices.push_back(i);
			break;
		case type_sprite:
			m_sprite_data.m_indices.push_back(i);
			break;
		case type_text:
			m_text_data.m_indices.push_back(i);
			break;
		}
	}

} //- kokoro