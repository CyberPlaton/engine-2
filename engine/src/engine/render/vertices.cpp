#include <engine/render/vertices.hpp>

namespace kokoro
{
	bgfx::VertexLayout spos_uv_vertex::S_LAYOUT;
	bgfx::VertexLayoutHandle spos_uv_vertex::S_HANDLE;
	bgfx::VertexLayout spos_color_uv_vertex::S_LAYOUT;
	bgfx::VertexLayoutHandle spos_color_uv_vertex::S_HANDLE;

	//------------------------------------------------------------------------------------------------------------------------
	void spos_uv_vertex::init()
	{
		S_LAYOUT.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();

		S_HANDLE = bgfx::createVertexLayout(S_LAYOUT);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void spos_color_uv_vertex::init()
	{
		S_LAYOUT.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();

		S_HANDLE = bgfx::createVertexLayout(S_LAYOUT);
	}

} //- kokoro