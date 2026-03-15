#pragma once
#include <engine/math/vec3.hpp>
#include <engine/math/mat4.hpp>
#include <engine/render/vertices.hpp>
#include <engine/effect/effect.hpp>
#include <vector>

namespace kokoro
{
	constexpr uint64_t C_MATERIAL_STATE_DEFAULT = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW
		| BGFX_STATE_MSAA
		;

	//------------------------------------------------------------------------------------------------------------------------
	class cdebug_drawer final
	{
	public:
		//- The type basically specifies what we will be drawing, and anticipate which
		//- vertex definition is ought to be used. Moreover this divides the render states
		//- for each separate render type
		enum type : uint8_t
		{
			type_none = 0,				//- Default, shapes and basic primitives
			type_sprite,				//- Used for drawing textures
			type_text,					//- Used for drawing text

			type_count,
		};

		enum blending_mode : uint8_t
		{
			blending_mode_none = 0,
			blending_mode_alpha,
			blending_mode_additive,
			blending_mode_multiplicative,
			blending_mode_premultiplied,
			blending_mode_screen,
			blending_mode_darken,
		};

		enum culling_mode : uint8_t
		{
			culling_mode_none = 0,
			culling_mode_cw,
			culling_mode_ccw,
		};

		enum depth_test_mode : uint8_t
		{
			depth_test_mode_none = 0,
			depth_test_mode_less,
			depth_test_mode_less_equal,
			depth_test_mode_equal,
			depth_test_mode_greater_equal,
			depth_test_mode_greater,
			depth_test_mode_not_equal,
			depth_test_mode_never,
			depth_test_mode_always,
		};

		enum render_effect : uint8_t
		{
			render_effect_none = 0,
		};

		cdebug_drawer() = default;
		~cdebug_drawer() = default;

		bool					init();
		void					shutdown();

		cdebug_drawer&			begin(bgfx::ViewId view);			//- Prepare for drawing of a new frame
		cdebug_drawer&			frame();							//- Submit everything we gathered so far

		cdebug_drawer&			render_type(type value);			//- Set the current render type we want to be working on, note that this does not enforce a flush
		cdebug_drawer&			depth_test(depth_test_mode mode);	//- Set the depth test mode to be used for rendering state
		cdebug_drawer&			cull(culling_mode mode);			//- Set the culling mode to be used for rendering state
		cdebug_drawer&			blend(blending_mode mode);			//- Set the blending mode to be used for rendering state
		cdebug_drawer&			effect(render_effect value);		//- Set the effect to be used for rendering, as fallback a default effect is used
		cdebug_drawer&			submit() { flush(); return *this; }
		cdebug_drawer&			draw(const math::vec3_t& v0, const math::vec3_t& v1, const math::vec3_t& v2)
		{
			auto new_state = state().m_state;
			new_state &= ~BGFX_STATE_PT_MASK;
			set_state(new_state);

			push_vertex(v0.x, v0.y, v0.z, state().m_color);
			push_vertex(v1.x, v1.y, v1.z, state().m_color);
			push_vertex(v2.x, v2.y, v2.z, state().m_color);
			return *this;
		}

		cdebug_drawer&			line(const math::vec3_t& v0, const math::vec3_t& v1)
		{
			auto new_state = state().m_state;
			new_state &= ~BGFX_STATE_PT_MASK;
			new_state |= BGFX_STATE_PT_LINES;
			set_state(new_state);

			push_vertex(v0.x, v0.y, v0.z, state().m_color);
			push_vertex(v1.x, v1.y, v1.z, state().m_color);
		}

	private:
		struct srender_state
		{
			math::mat4_t m_matrix		= math::mat4_t(1.0f);
			uint64_t m_state			= C_MATERIAL_STATE_DEFAULT;
			uint32_t m_color			= 0xffffffff;
			bgfx::ViewId m_view			= std::numeric_limits<bgfx::ViewId>::max();
			resource_handle_t m_effect	= invalid_handle_t;
		};

		struct sshape_data
		{
			srender_state m_state;
			std::vector<spos_color_vertex> m_vertices;
			std::vector<uint16_t> m_indices;
		};

		struct ssprite_data
		{
			srender_state m_state;
			std::vector<spos_color_uv_vertex> m_vertices;
			std::vector<uint16_t> m_indices;
		};

		struct stext_data
		{
			srender_state m_state;
			std::vector<spos_color_uv_vertex> m_vertices;
			std::vector<uint16_t> m_indices;
		};

		type m_current_type = type_none;
		sshape_data m_shape_data;
		ssprite_data m_sprite_data;
		stext_data m_text_data;

	private:
		//- Retrieve the current relevant state for our current drawing type
		srender_state&				state();
		void						set_state(uint64_t new_state = 0);
		auto						indices_vector() -> std::vector<uint16_t>&;
		void						vertex_clear();
		void*						vertex_data() const;
		uint64_t					vertex_count() const;
		const bgfx::VertexLayout&	vertex_layout() const;
		bgfx::VertexLayoutHandle	vertex_handle() const;
		bool						submit_buffers();
		void						flush();

		template<typename... ARGS>
		void						push_vertex(ARGS... args)
		{
			switch (m_current_type)
			{
			default:
			case type_none:
				m_shape_data.m_vertices.push_back({ std::forward<ARGS>(args)... });
				break;
			case type_sprite:
				m_sprite_data.m_vertices.push_back({ std::forward<ARGS>(args)... });
				break;
			case type_text:
				m_text_data.m_vertices.push_back({ std::forward<ARGS>(args)... });
				break;
			}
		}
	};

} //- kokoro