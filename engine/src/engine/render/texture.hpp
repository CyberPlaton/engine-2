#pragma once
#include <bgfx.h>
#include <bimg.h>
#include <string>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	struct stexture final
	{
		using format = bgfx::TextureFormat::Enum;

		bimg::ImageContainer m_image;
		bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct stexture_snapshot final
	{
		std::string m_texture;
	};

	stexture_snapshot*	texture_snapshot_from_file(const char* filepath);
	stexture*			instantiate_texture(const stexture_snapshot& snapshot, const char* name);
	void				destroy_texture(const char* name);

} //- kokoro