#include <engine/effect/effect.hpp>
#include <engine/effect/effect_parser.hpp>
#include <engine/effect/shader.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/io/io.hpp>
#include <core/mutex.hpp>
#include <registrator.hpp>
#include <engine.hpp>
#include <unordered_map>

namespace kokoro
{
	namespace
	{
		std::unordered_map<const char*, seffect> instance_cache;
		std::unordered_map<const char*, seffect_snapshot> snapshot_cache;
		core::cmutex instance_mutex;
		core::cmutex snapshot_mutex;

		//------------------------------------------------------------------------------------------------------------------------
		std::string default_include_handler(const char* filepath)
		{
			if (filepath_exists(filepath))
			{
				filepath_t path(filepath);
				auto& vfs = instance().service<cvirtual_filesystem_service>();

				if (auto file = vfs.open(path, file_options_read | file_options_text); file)
				{
					auto future = file->read_async();

					while (future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::ready) {}
					file->close();

					auto mem = future.get();

					if (mem && !mem->empty())
					{
						return std::string(mem->data(), mem->size());
					}
				}
			}
			return {};
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	seffect* instantiate_effect(const seffect_snapshot& snapshot, const char* name)
	{
		seffect effect;
		auto& vfs = instance().service<cvirtual_filesystem_service>();

		const auto load_file = [&](const filepath_t& path) -> core::memory_ref_t
			{
				if (auto file = vfs.open(path, file_options_read | file_options_text); file)
				{
					auto future = file->read_async();

					while (future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::ready) {}
					file->close();

					auto mem = future.get();

					if (mem && !mem->empty())
					{
						return mem;
					}
				}
				return nullptr;
			};

		//- Create the effect
		{
			//- Check whether we are loading an 'fx' file that contains a pixel and vertex shader in one file,
			//- or just a regular situation with one shader per file.
			if (snapshot.m_vs.m_filepath_or_name == snapshot.m_ps.m_filepath_or_name)
			{
				if (!filepath_exists(snapshot.m_vs.m_filepath_or_name))
				{
					//- Try resolving filepath using virtual file system
				}

				if (filepath_exists(snapshot.m_vs.m_filepath_or_name))
				{
					filepath_t fx_filepath = snapshot.m_vs.m_filepath_or_name;

					if (auto mem = load_file(fx_filepath); mem && !mem->empty())
					{
						ceffect_parser parser(mem->data(), &default_include_handler);
						const auto output = parser.parse();

						if (!output.m_vs.empty() && !output.m_ps.empty())
						{
							//- Load vertex shader
							{
								scompile_options options;
								options.m_name = fx_filepath.filename().generic_string();
								options.m_type = scompile_options::shader_type_vertex;
								options.m_include_dirs.push_back(fx_filepath.parent_path().generic_string());
								effect.m_vs.m_data = compile_shader_from_string(output.m_vs.c_str(), options);
								effect.m_vs.m_handle = bgfx::createShader(bgfx::makeRef(effect.m_vs.m_data->data(), effect.m_vs.m_data->size()));
							}

							//- Load pixel shader
							{
								scompile_options options;
								options.m_name = fx_filepath.filename().generic_string();
								options.m_type = scompile_options::shader_type_pixel;
								options.m_include_dirs.push_back(fx_filepath.parent_path().generic_string());
								effect.m_ps.m_data = compile_shader_from_string(output.m_ps.c_str(), options);
								effect.m_ps.m_handle = bgfx::createShader(bgfx::makeRef(effect.m_ps.m_data->data(), effect.m_ps.m_data->size()));
							}
						}
					}
				}
			}
			else
			{
				//- Load vertex shader
				{
					const auto filepath = filepath_t(snapshot.m_vs.m_filepath_or_name);

					scompile_options options;
					options.m_name = filepath.filename().generic_string();
					options.m_type = scompile_options::shader_type_vertex;
					options.m_include_dirs.push_back(filepath.parent_path().generic_string());

					switch (snapshot.m_vs.m_type)
					{
					case seffect_snapshot::type_file:
					{
						if (auto mem = load_file(filepath); mem && !mem->empty())
						{
							ceffect_parser parser(mem->data(), &default_include_handler);
							const auto output = parser.parse();

							if (!output.m_vs.empty())
							{
								effect.m_vs.m_data = compile_shader_from_string(output.m_vs.c_str(), options);
								effect.m_vs.m_handle = bgfx::createShader(bgfx::makeRef(effect.m_vs.m_data->data(), effect.m_vs.m_data->size()));
							}
						}
						break;
					}
					case seffect_snapshot::type_embedded:
					{
						return nullptr;
						break;
					}
					default:
					case seffect_snapshot::type_none:
						return nullptr;
					}
				}

				//- Load pixel shader
				{
					const auto filepath = filepath_t(snapshot.m_ps.m_filepath_or_name);

					scompile_options options;
					options.m_name = filepath.filename().generic_string();
					options.m_type = scompile_options::shader_type_pixel;
					options.m_include_dirs.push_back(filepath.parent_path().generic_string());

					switch (snapshot.m_ps.m_type)
					{
					case seffect_snapshot::type_file:
					{
						if (auto mem = load_file(filepath); mem && !mem->empty())
						{
							ceffect_parser parser(mem->data(), &default_include_handler);
							const auto output = parser.parse();

							if (!output.m_ps.empty())
							{
								effect.m_ps.m_data = compile_shader_from_string(output.m_ps.c_str(), options);
								effect.m_ps.m_handle = bgfx::createShader(bgfx::makeRef(effect.m_ps.m_data->data(), effect.m_ps.m_data->size()));
							}
						}
						break;
					}
					case seffect_snapshot::type_embedded:
					{
						return nullptr;
						break;
					}
					default:
					case seffect_snapshot::type_none:
						return nullptr;
					}
				}

				//- Finally, create the program from created shaders
				effect.m_program = bgfx::createProgram(effect.m_vs.m_handle, effect.m_ps.m_handle);
			}

			//- Load uniform data
			for (const auto& snapshot_uniform : snapshot.m_uniforms)
			{
				auto& uniform = effect.m_uniforms.emplace_back();
				uniform = snapshot_uniform;
				uniform.m_handle = bgfx::createUniform(uniform.m_name.c_str(), uniform.m_type);
				if (uniform.m_data.is_valid())
				{
					bgfx::setUniform(uniform.m_handle, uniform.m_data.get_raw_ptr());
				}
			}
		}


		core::cscoped_mutex m(instance_mutex);

		//- Store created effect to instance cache
		if (auto [it, result] = instance_cache.emplace(std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(std::move(effect))); result)
		{
			return &it->second;
		}
		return nullptr;
	}

	//- Load and cache an effect snapshot from disk
	//------------------------------------------------------------------------------------------------------------------------
	seffect_snapshot* effect_snapshot_from_file(const char* filepath)
	{
		if (const auto it = snapshot_cache.find(filepath); it != snapshot_cache.end())
		{
			return &it->second;
		}

		if (filepath_exists(filepath))
		{
			filepath_t path(filepath);

			auto& vfs = instance().service<cvirtual_filesystem_service>();

			if (auto file = vfs.open(path, file_options_read | file_options_text); file)
			{
				auto future = file->read_async();

				while (future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::ready) {}
				file->close();

				auto mem = future.get();

				if (mem && !mem->empty())
				{
					if (auto var = kokoro::from_json_blob(rttr::type::get<seffect_snapshot>(), mem->data(), mem->size()); var.is_valid())
					{
						core::cscoped_mutex m(snapshot_mutex);

						if (auto [it, result] = snapshot_cache.emplace(std::piecewise_construct,
							std::forward_as_tuple(filepath),
							std::forward_as_tuple(std::move(var.get_value<seffect_snapshot>()))); result)
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
	void destroy_instance(const char* name)
	{
		if (auto it = instance_cache.find(name); it != instance_cache.end())
		{
			auto& effect = it->second;

			//- Destroy the uniforms if effect is using any
			for (auto& uniform : effect.m_uniforms)
			{
				if (bgfx::isValid(uniform.m_handle))
				{
					bgfx::destroy(uniform.m_handle);
				}
			}

			//- Destroy the vertex and pixel shaders
			if (bgfx::isValid(effect.m_vs.m_handle))
			{
				bgfx::destroy(effect.m_vs.m_handle);
			}
			if (bgfx::isValid(effect.m_ps.m_handle))
			{
				bgfx::destroy(effect.m_ps.m_handle);
			}
			if (bgfx::isValid(effect.m_program))
			{
				bgfx::destroy(effect.m_program);
			}

			core::cscoped_mutex m(instance_mutex);

			instance_cache.erase(name);
		}
	}

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro;

	rttr::detail::default_constructor<std::vector<suniform>>();

	rttr::cregistrator<suniform>("suniform")
		.prop("m_name", &suniform::m_name)
		.prop("m_data", &suniform::m_data)
		.prop("m_type", &suniform::m_type);

	rttr::cregistrator<seffect_snapshot::sshader>("seffect_snapshot::sshader")
		.prop("m_filepath_or_name", &seffect_snapshot::sshader::m_filepath_or_name)
		.prop("m_type", &seffect_snapshot::sshader::m_type);

	rttr::cregistrator<seffect_snapshot>("seffect_snapshot")
		.prop("m_vs", &seffect_snapshot::m_vs)
		.prop("m_ps", &seffect_snapshot::m_ps)
		.prop("m_uniforms", &seffect_snapshot::m_uniforms);

}