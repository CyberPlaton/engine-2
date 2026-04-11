#include <engine/render/texture.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <engine.hpp>
#include <core/registrator.hpp>
#include <fmt.h>
#include <rttr.h>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	std::optional<stexture> stexture::load(const rttr::variant& snapshot)
	{
		if (!snapshot.is_valid())
		{
			log::err("stexture::load - received invalid snapshot variant");
			return std::nullopt;
		}

		const auto& snaps = snapshot.get_value<stexture_snapshot>();
		auto& vfs = instance().service<cvirtual_filesystem_service>();
		filepath_t path(snaps.m_texture);

		if (!vfs.exists(path))
		{
			//- Try resolving filepath using virtual file system
			if (const auto [result, p] = vfs.resolve(path); result)
			{
				path = p;
			}
			else
			{
				instance().service<clog_service>().err(fmt::format("Could not find texture file at '{}'",
					snaps.m_texture).c_str());
				return std::nullopt;
			}
		}

		if (auto file = vfs.open(path, file_options_read | file_options_binary); file.opened())
		{
			auto mem = file.read_sync();

			if (mem && !mem->empty())
			{
				stexture texture;
				bx::DefaultAllocator allocator;

				if (auto* image = bimg::imageParse(&allocator, mem->data(), mem->size()); image)
				{
					constexpr uint64_t C_TEXTURE_SAMPLER_FLAGS_DEFAULT = 0
						| BGFX_SAMPLER_UVW_CLAMP
						| BGFX_SAMPLER_POINT
						;

					//- Copy container, but do not keep reference of the image data, as of now we free everything and
					//- use only the texture
					texture.m_image = *image;
					texture.m_image.m_allocator = nullptr;
					texture.m_image.m_data = nullptr;

					auto* mem = bgfx::copy(image->m_data, image->m_size);
					bimg::imageFree(image);

					if (texture.m_handle = bgfx::createTexture2D((uint16_t)texture.m_image.m_width, (uint16_t)texture.m_image.m_height,
						texture.m_image.m_numMips > 1, texture.m_image.m_numLayers, (texture_format_t)texture.m_image.m_format,
						C_TEXTURE_SAMPLER_FLAGS_DEFAULT, mem); bgfx::isValid(texture.m_handle))
					{
						return std::move(texture);
					}
				}
			}
		}
		return std::nullopt;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void stexture::unload(stexture& texture)
	{
		if (bgfx::isValid(texture.m_handle))
		{
			bgfx::destroy(texture.m_handle);
		}
	}

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<stexture_snapshot>("stexture_snapshot")
		.prop("m_texture", &stexture_snapshot::m_texture);
}