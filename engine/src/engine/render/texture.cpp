#include <engine/render/texture.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/mutex.hpp>
#include <core/io.hpp>
#include <engine.hpp>
#include <fmt.h>
#include <rttr.h>

namespace kokoro
{
	namespace
	{
		std::unordered_map<const char*, stexture> instance_cache;
		std::unordered_map<const char*, stexture_snapshot> snapshot_cache;
		core::cmutex instance_mutex;
		core::cmutex snapshot_mutex;

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	stexture_snapshot* texture_snapshot_from_file(const char* filepath)
	{
		if (const auto it = snapshot_cache.find(filepath); it != snapshot_cache.end())
		{
			return &it->second;
		}

		auto& vfs = instance().service<cvirtual_filesystem_service>();
		filepath_t path(filepath);

		if (!vfs.exists(path))
		{
			//- Try resolving filepath using virtual file system
			if (const auto [result, p] = vfs.resolve(path); result)
			{
				path = p;
			}
			else
			{
				instance().service<clog_service>().err(fmt::format("Could not find texture snapshot file at '{}'",
					filepath).c_str());
				return nullptr;
			}
		}

		if (auto file = vfs.open(path, file_options_read | file_options_text); file)
		{
			auto future = file->read_async();

			while (future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::ready) {}
			file->close();

			auto mem = future.get();

			if (mem && !mem->empty())
			{
				if (auto var = core::from_json_blob(rttr::type::get<stexture_snapshot>(), mem->data(), mem->size()); var.is_valid())
				{
					core::cscoped_mutex m(snapshot_mutex);

					if (auto [it, result] = snapshot_cache.emplace(std::piecewise_construct,
						std::forward_as_tuple(filepath),
						std::forward_as_tuple(std::move(var.get_value<stexture_snapshot>()))); result)
					{
						return &it->second;
					}
				}
			}
		}
		return nullptr;
	}

	//------------------------------------------------------------------------------------------------------------------------
	stexture* instantiate_texture(const stexture_snapshot& snapshot, const char* name)
	{
		if (const auto it = instance_cache.find(name); it != instance_cache.end())
		{
			return &it->second;
		}

		auto& vfs = instance().service<cvirtual_filesystem_service>();
		filepath_t path(snapshot.m_texture);

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
					snapshot.m_texture).c_str());
				return nullptr;
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
						core::cscoped_mutex m(instance_mutex);

						//- Store created texture to instance cache
						if (auto [it, result] = instance_cache.emplace(std::piecewise_construct,
							std::forward_as_tuple(name),
							std::forward_as_tuple(std::move(texture))); result)
						{
							return &it->second;
						}
					}
				}
			}
		}
		return nullptr;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void destroy_texture(const char* name)
	{
		if (auto it = instance_cache.find(name); it != instance_cache.end())
		{
			auto& texture = it->second;

			if (bgfx::isValid(texture.m_handle))
			{
				bgfx::destroy(texture.m_handle);
			}

			core::cscoped_mutex m(instance_mutex);

			instance_cache.erase(name);
		}
	}

} //- kokoro