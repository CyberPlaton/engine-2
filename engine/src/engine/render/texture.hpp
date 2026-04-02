#pragma once
#include <engine/services/resource_manager_service.hpp>
#include <bgfx.h>
#include <bimg.h>
#include <string>
#include <optional>

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
		static std::optional<stexture>	load(const rttr::variant& snapshot);
		static void						unload(stexture& texture);

		using format = bgfx::TextureFormat::Enum;

		bimg::ImageContainer m_image;
		bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;
	};

} //- kokoro