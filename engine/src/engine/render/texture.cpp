#include <engine/render/texture.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <engine.hpp>
#include <fmt.h>
#include <rttr.h>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	stexture ctexture_resource_manager_service::do_instantiate(const stexture_snapshot* snaps)
	{
		auto& vfs = instance().service<cvirtual_filesystem_service>();
		filepath_t path(snaps->m_texture);

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
					snaps->m_texture).c_str());
				return {};
			}
		}

		if (auto file = vfs.open(path, file_options_read | file_options_binary); file)
		{
			auto future = file->read_async();

			while (future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::ready) {}
			file->close();

			auto mem = future.get();

			if (mem && !mem->empty())
			{
				stexture texture;

				if (bimg::imageParse(texture.m_image, mem->data(), mem->size()))
				{
					const auto* mem = bgfx::makeRef(texture.m_image.m_data, texture.m_image.m_size);

					constexpr uint64_t C_TEXTURE_SAMPLER_FLAGS_DEFAULT = 0
						| BGFX_SAMPLER_UVW_CLAMP
						| BGFX_SAMPLER_POINT
						;

					if (texture.m_handle = bgfx::createTexture2D((uint16_t)texture.m_image.m_width, (uint16_t)texture.m_image.m_height,
						texture.m_image.m_numMips > 1, texture.m_image.m_numLayers, (bgfx::TextureFormat::Enum)texture.m_image.m_format,
						C_TEXTURE_SAMPLER_FLAGS_DEFAULT, mem); bgfx::isValid(texture.m_handle))
					{
						return texture;
					}
				}
			}
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ctexture_resource_manager_service::do_destroy(stexture* inst)
	{
		if (bgfx::isValid(inst->m_handle))
		{
			bgfx::destroy(inst->m_handle);
		}
	}

} //- kokoro