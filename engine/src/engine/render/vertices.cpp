#include <engine/render/vertices.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	void spos_uv_vertex::init()
	{
		auto& l = layout();
		auto& h = handle();

		l.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float).end();

		h = bgfx::createVertexLayout(l);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void spos_color_uv_vertex::init()
	{
		auto& l = layout();
		auto& h = handle();

		l.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float).end();

		h = bgfx::createVertexLayout(l);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void spos_color_vertex::init()
	{
		auto& l = layout();
		auto& h = handle();

		l.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true).end();

		h = bgfx::createVertexLayout(l);
	}

} //- kokoro