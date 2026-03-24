#include <engine/effect/effect.hpp>
#include <engine/effect/effect_parser.hpp>
#include <engine/effect/embedded_shaders.hpp>
#include <engine/effect/shader.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/io.hpp>
#include <core/mutex.hpp>
#include <core/registrator.hpp>
#include <engine.hpp>
#include <fmt.h>
#include <unordered_map>

#pragma optimize("", off)
namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	std::pair<bool, seffect> seffect::load(const rttr::variant& snapshot)
	{
		const auto& snaps = snapshot.get_value<seffect_snapshot>();
		seffect effect;
		auto& vfs = instance().service<cvirtual_filesystem_service>();

		const auto load_file = [&](const filepath_t& path) -> core::memory_ref_t
			{
				if (auto wrapper = vfs.open(path, file_options_read | file_options_text); wrapper)
				{
					auto& file = wrapper.get();
					auto mem = file.read_sync();
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
			if (snaps.m_vs.m_filepath_or_name == snaps.m_ps.m_filepath_or_name)
			{
				filepath_t fx_filepath = snaps.m_vs.m_filepath_or_name;

				if (!vfs.exists(fx_filepath))
				{
					//- Try resolving filepath using virtual file system
					if (const auto [result, p] = vfs.resolve(fx_filepath); result)
					{
						fx_filepath = p;
					}
					else
					{
						instance().service<clog_service>().err(fmt::format("Could not find include file at '{}'",
							snaps.m_vs.m_filepath_or_name).c_str());
						return { false, {} };
					}
				}

				if (auto mem = load_file(fx_filepath); mem && !mem->empty())
				{
					auto& log = instance().service<clog_service>();
					log.debug(fmt::format("\tshader source code:\n'{}'",
						mem->data()).c_str());

					ceffect_parser parser(mem->data());
					const auto output = parser.parse();

					log.debug(fmt::format("\tvertex output code:\n'{}'",
						output.m_vs).c_str());

					log.debug(fmt::format("\tpixel output code:\n'{}'",
						output.m_ps).c_str());


					if (!output.m_vs.empty() && !output.m_ps.empty())
					{
						//- Load vertex shader
						{
							scompile_options options;
							options.m_name = fx_filepath.filename().generic_string();
							options.m_type = scompile_options::shader_type_vertex;
							options.m_include_dirs.push_back(fx_filepath.parent_path().generic_string());
							auto mem = compile_shader_from_string(output.m_vs.c_str(), options);
							effect.m_vs.m_handle = bgfx::createShader(bgfx::copy(mem->data(), mem->size()));
						}

						//- Load pixel shader
						{
							scompile_options options;
							options.m_name = fx_filepath.filename().generic_string();
							options.m_type = scompile_options::shader_type_pixel;
							options.m_include_dirs.push_back(fx_filepath.parent_path().generic_string());
							auto mem = compile_shader_from_string(output.m_ps.c_str(), options);
							effect.m_ps.m_handle = bgfx::createShader(bgfx::copy(mem->data(), mem->size()));
						}
					}
				}
			}
			else
			{
				//- Load vertex shader
				{
					const auto filepath = filepath_t(snaps.m_vs.m_filepath_or_name);

					scompile_options options;
					options.m_name = filepath.filename().generic_string();
					options.m_type = scompile_options::shader_type_vertex;
					options.m_include_dirs.push_back(filepath.parent_path().generic_string());

					switch (snaps.m_vs.m_type)
					{
					case seffect_snapshot::type_file:
					{
						if (auto mem = load_file(filepath); mem && !mem->empty())
						{
							ceffect_parser parser(mem->data());
							const auto output = parser.parse();

							if (!output.m_vs.empty())
							{
								auto mem = compile_shader_from_string(output.m_vs.c_str(), options);
								effect.m_vs.m_handle = bgfx::createShader(bgfx::copy(mem->data(), mem->size()));
							}
						}
						break;
					}
					case seffect_snapshot::type_embedded:
					{
						const char* code = sembedded_shaders::get(snaps.m_vs.m_filepath_or_name.c_str());
						ceffect_parser parser(code);
						const auto output = parser.parse();

						if (!output.m_vs.empty())
						{
							auto mem = compile_shader_from_string(output.m_vs.c_str(), options);
							effect.m_vs.m_handle = bgfx::createShader(bgfx::copy(mem->data(), mem->size()));
						}
						break;
					}
					default:
					case seffect_snapshot::type_none:
						return {};
					}
				}

				//- Load pixel shader
				{
					const auto filepath = filepath_t(snaps.m_ps.m_filepath_or_name);

					scompile_options options;
					options.m_name = filepath.filename().generic_string();
					options.m_type = scompile_options::shader_type_pixel;
					options.m_include_dirs.push_back(filepath.parent_path().generic_string());

					switch (snaps.m_ps.m_type)
					{
					case seffect_snapshot::type_file:
					{
						if (auto mem = load_file(filepath); mem && !mem->empty())
						{
							ceffect_parser parser(mem->data());
							const auto output = parser.parse();

							if (!output.m_ps.empty())
							{
								auto mem = compile_shader_from_string(output.m_ps.c_str(), options);
								effect.m_ps.m_handle = bgfx::createShader(bgfx::copy(mem->data(), mem->size()));
							}
						}
						break;
					}
					case seffect_snapshot::type_embedded:
					{
						const char* code = sembedded_shaders::get(snaps.m_ps.m_filepath_or_name.c_str());
						ceffect_parser parser(code);
						const auto output = parser.parse();

						if (!output.m_ps.empty())
						{
							auto mem = compile_shader_from_string(output.m_ps.c_str(), options);
							effect.m_ps.m_handle = bgfx::createShader(bgfx::copy(mem->data(), mem->size()));
						}
						break;
					}
					default:
					case seffect_snapshot::type_none:
						return {};
					}
				}
			}

			//- Finally, create the program from created shaders
			if (bgfx::isValid(effect.m_vs.m_handle) && bgfx::isValid(effect.m_ps.m_handle))
			{
				effect.m_program = bgfx::createProgram(effect.m_vs.m_handle, effect.m_ps.m_handle);
			}
		}
		return { true, effect };
	}

	//------------------------------------------------------------------------------------------------------------------------
	void seffect::unload(seffect& effect)
	{
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
	}

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro;

	rttr::registration::enumeration<seffect_snapshot::type>("seffect_snapshot::type")
		(
			rttr::value("type_none", seffect_snapshot::type_none),
			rttr::value("type_file", seffect_snapshot::type_file),
			rttr::value("type_embedded", seffect_snapshot::type_embedded)
		);
	rttr::cregistrator<seffect_snapshot::ssnapshot_shader>("seffect_snapshot::sshader")
		.prop("m_filepath_or_name", &seffect_snapshot::ssnapshot_shader::m_filepath_or_name)
		.prop("m_type", &seffect_snapshot::ssnapshot_shader::m_type);

	rttr::cregistrator<seffect_snapshot>("seffect_snapshot")
		.prop("m_vs", &seffect_snapshot::m_vs)
		.prop("m_ps", &seffect_snapshot::m_ps);

}