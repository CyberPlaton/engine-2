#pragma once
#include <bgfx.h>

//- Utility macro for a vertex type. Creates a default implementation and only the init function requires attention.
//------------------------------------------------------------------------------------------------------------------------
#define DECLARE_VERTEX_TYPE(t) \
static t&					get() \
{ \
	static t S_STATIC; \
	if (!bgfx::isValid(S_STATIC.handle())) \
	{ \
		S_STATIC.init(); \
	} \
	return S_STATIC; \
} \
bgfx::VertexLayout&			layout() \
{ \
	static bgfx::VertexLayout S_LAYOUT = {}; \
	return S_LAYOUT; \
} \
bgfx::VertexLayoutHandle&	handle() \
{ \
	static bgfx::VertexLayoutHandle S_HANDLE = BGFX_INVALID_HANDLE; \
	return S_HANDLE; \
}

namespace kokoro
{
	//- Vertex with vec3 position and uint32 color
	//------------------------------------------------------------------------------------------------------------------------
	struct spos_color_vertex final
	{
		DECLARE_VERTEX_TYPE(spos_color_vertex);

		void init();

		float m_x;
		float m_y;
		float m_z;
		unsigned m_abgr;
	};

	//- Vertex with vec3 position and vec2 UV coordinate
	//------------------------------------------------------------------------------------------------------------------------
	struct spos_uv_vertex final
	{
		DECLARE_VERTEX_TYPE(spos_uv_vertex);

		void init();

		float m_x;
		float m_y;
		float m_z;
		float m_u;
		float m_v;
	};

	//- Vertex with vec3 position, uint32 color and vec2 UV coordinate
	//------------------------------------------------------------------------------------------------------------------------
	struct spos_color_uv_vertex final
	{
		DECLARE_VERTEX_TYPE(spos_color_uv_vertex);

		void init();

		float m_x;
		float m_y;
		float m_z;
		unsigned m_abgr;
		float m_u;
		float m_v;
	};

} //- kokoro