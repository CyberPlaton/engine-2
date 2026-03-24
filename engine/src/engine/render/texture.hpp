#pragma once
#include <bgfx.h>
#include <bimg.h>
#include <string>
#include <engine/services/resource_manager_service.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	struct stexture_snapshot final
	{
		std::string m_texture;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct stexture
	{
		static std::pair<bool, stexture>	load(const rttr::variant& snapshot);
		static void							unload(stexture& texture);

		using format = bgfx::TextureFormat::Enum;

		bimg::ImageContainer m_image;
		bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;
	};

} //- kokoro