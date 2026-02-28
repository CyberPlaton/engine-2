#pragma once
#include <bgfx.h>

namespace kokoro
{
	//- Vertex with vec3 position and vec2 UV coordinate
	//------------------------------------------------------------------------------------------------------------------------
	struct spos_uv_vertex final
	{
		static bgfx::VertexLayout S_LAYOUT;
		static bgfx::VertexLayoutHandle S_HANDLE;

		static void init();

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
		static bgfx::VertexLayout S_LAYOUT;
		static bgfx::VertexLayoutHandle S_HANDLE;

		static void init();

		float m_x;
		float m_y;
		float m_z;
		unsigned m_abgr;
		float m_u;
		float m_v;
	};

} //- kokoro