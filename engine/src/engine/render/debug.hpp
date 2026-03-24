#pragma once
#include <engine/math/vec3.hpp>
#include <engine/math/mat4.hpp>
#include <engine/render/vertices.hpp>
#include <engine/effect/effect.hpp>
#include <vector>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class cdebug_drawer final
	{
	public:
		inline static constexpr uint64_t C_STATE_DEFAULT = 0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_MSAA;

		enum blending_mode : uint8_t
		{
			blending_mode_none = 0,
			blending_mode_alpha,			//- Using the alpha channel, interpolate between source and destination colors
			blending_mode_additive,			//- Basic adding of colors
			blending_mode_multiplicative,	//- Basic multiply of colors
			blending_mode_premultiplied,	//- Same as alpha, without multiplying source color by alpha
			blending_mode_screen,
			blending_mode_darken,
			blending_mode_lighten,
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

		cdebug_drawer();
		~cdebug_drawer() = default;

		bool						init();
		void						shutdown();

		cdebug_drawer&				begin(bgfx::ViewId view,															//- Prepare for drawing of a new frame
										bgfx::FrameBufferHandle fbh,
										uint16_t flags = 0,
										uint32_t color = 0,
										float depth = 1.0f,
										uint8_t stencil = 0);
		void						frame();																			//- Submit everything we gathered for all render types

		cdebug_drawer&				view_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);							//- Set view region. Primitives outside will be clipped
		cdebug_drawer&				depth_test(depth_test_mode mode);													//- Set the depth test mode to be used for rendering state
		cdebug_drawer&				cull(culling_mode mode);															//- Set the culling mode to be used for rendering state
		cdebug_drawer&				blend(blending_mode mode);															//- Set the blending mode to be used for rendering state
		cdebug_drawer&				effect(render_effect value);														//- Set the effect to be used for rendering, as fallback a default effect is used
		cdebug_drawer&				submit() { flush(); return *this; }													//- Submit what we gathered so far for current render type
		cdebug_drawer&				color(uint32_t value);																//- Color that will be set for each submitted vertex
		cdebug_drawer&				wireframe(const bool value);														//- Whether to fill the object with color or interpret as lines

		cdebug_drawer&				transform(const math::mat4_t& mtx);													//- Matrix used for transforming each submitted vertex
		cdebug_drawer&				translate(const math::vec3_t& value);												//- Set translation for transformation matrix
		cdebug_drawer&				scale(const math::vec3_t& value);													//- Set scale for transformation matrix
		cdebug_drawer&				rotate(const math::vec3_t& value);													//- Set rotation for transformation matrix

		//- TODO:
		//- Our base functions should be to submit many triangles, many lines and many quads and not single ones,
		//- and the single variants should basically submit a vector of 1.

		cdebug_drawer&				triangle(const math::vec3_t& v0, const math::vec3_t& v1,							//- Submit a single triangle
										const math::vec3_t& v2, uint32_t color = 0);
		cdebug_drawer&				quad(const math::vec3_t& v0, const math::vec3_t& v1,								//- Submit a single quad
									const math::vec3_t& v2, const math::vec3_t& v3, uint32_t color = 0);

		cdebug_drawer&				line(const math::vec3_t& v0, const math::vec3_t& v1, uint32_t color = 0);			//- Submit a single line

	private:
		struct srender_state
		{
			math::mat4_t m_matrix				= math::mat4_t(1.0f);
			uint64_t m_state					= C_STATE_DEFAULT;
			uint32_t m_color					= 0xffffffff;
			bgfx::FrameBufferHandle m_fbh		= BGFX_INVALID_HANDLE;
			bgfx::ViewId m_view					= std::numeric_limits<bgfx::ViewId>::max();
			uint16_t m_clear_flags				= 0;
			uint32_t m_clear_color				= 0;
			float m_clear_depth					= 0.0f;
			uint8_t m_clear_stencil				= 0;
			cview<seffect> m_effect;
			uint16_t m_view_x					= 0;
			uint16_t m_view_y					= 0;
			uint16_t m_view_w					= 0;
			uint16_t m_view_h					= 0;
			bool m_wireframe					= false;
		};

		srender_state m_state;
		std::vector<spos_color_uv_vertex> m_vertices;
		std::vector<uint16_t> m_indices;

	private:
		srender_state&				state();
		void						set_state(uint64_t value);
		auto						indices() -> std::vector<uint16_t>&;
		auto						vertices() -> std::vector<spos_color_uv_vertex>&;
		void*						vertex_data() const;
		uint32_t					vertex_count() const;
		const bgfx::VertexLayout&	vertex_layout() const;
		bgfx::VertexLayoutHandle	vertex_handle() const;
		void						submit_buffers();
		void						flush();
	};

} //- kokoro