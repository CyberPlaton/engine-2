#include <engine/render/debug.hpp>

namespace kokoro
{
	namespace
	{
		static const filepath_t C_DEFAULT_SHAPE_EFFECT_FILEPATH = "engine/effects/debug/default.effect";
		static const filepath_t C_DEFAULT_SPRITE_EFFECT_FILEPATH = "engine/effects/debug/default.effect";
		static const filepath_t C_DEFAULT_TEXT_EFFECT_FILEPATH = "engine/effects/debug/default.effect";
	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	bool cdebug_drawer::init()
	{
		auto& erm = instance().service<ceffect_resource_manager_service>();
		m_shape_data.m_state.m_effect = erm.instantiate(C_DEFAULT_SHAPE_EFFECT_FILEPATH);
		m_sprite_data.m_state.m_effect = erm.instantiate(C_DEFAULT_SHAPE_EFFECT_FILEPATH);
		m_text_data.m_state.m_effect = erm.instantiate(C_DEFAULT_SHAPE_EFFECT_FILEPATH);
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
	cdebug_drawer& cdebug_drawer::begin(bgfx::ViewId view)
	{
		state().m_view = view;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::frame()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::render_type(type value)
	{
		m_current_type = value == type_count ? type_none : value;
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer& cdebug_drawer::depth_test(depth_test_mode mode)
	{

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

	}

	//------------------------------------------------------------------------------------------------------------------------
	cdebug_drawer::srender_state& cdebug_drawer::state()
	{
		switch (m_current_type)
		{
		default:
		case type_none:
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
		case type_none:
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
		case type_none:
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
		case type_none:
			return (void*)m_shape_data.m_vertices.data();
		case type_sprite:
			return (void*)m_sprite_data.m_vertices.data();
		case type_text:
			return (void*)m_text_data.m_vertices.data();
		}
		return nullptr;
	}

	//------------------------------------------------------------------------------------------------------------------------
	uint64_t cdebug_drawer::vertex_count() const
	{
		switch (m_current_type)
		{
		default:
		case type_none:
			return m_shape_data.m_vertices.size();
		case type_sprite:
			return m_sprite_data.m_vertices.size();
		case type_text:
			return m_text_data.m_vertices.size();
		}
		return 0;
	}

	//------------------------------------------------------------------------------------------------------------------------
	const bgfx::VertexLayout& cdebug_drawer::vertex_layout() const
	{
		switch (m_current_type)
		{
		default:
		case type_none:
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
		case type_none:
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
				bx::memCopy(tvb.data, vertex_data(), count * sizeof(layout.m_stride));
				bgfx::setVertexBuffer(0, &tvb);

				const auto& indices = indices_vector();
				if (!indices.empty())
				{
					bgfx::TransientIndexBuffer tib;
					bgfx::allocTransientIndexBuffer(&tib, indices.size());
					bx::memCopy(tib.data, indices.data(), indices.size() * sizeof(uint16_t));
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
			set_state();

			bgfx::setTransform(state().m_matrix.value);

			const auto* effect = instance().service<ceffect_resource_manager_service>().get(state().m_effect);
			bgfx::submit(state().m_view, effect->m_program);
			bgfx::discard();
		}

		vertex_clear();
		indices_vector().clear();
	}

} //- kokoro