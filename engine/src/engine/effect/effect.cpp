#include <engine/effect/effect.hpp>
#include <engine/effect/effect_parser.hpp>
#include <engine/effect/shader.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/io.hpp>
#include <core/mutex.hpp>
#include <registrator.hpp>
#include <engine.hpp>
#include <fmt.h>
#include <unordered_map>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	seffect ceffect_resource_manager_service::do_instantiate(const seffect_snapshot* snaps)
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
			if (snaps->m_vs.m_filepath_or_name == snaps->m_ps.m_filepath_or_name)
			{
				filepath_t fx_filepath = snaps->m_vs.m_filepath_or_name;

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
							snaps->m_vs.m_filepath_or_name).c_str());
						return {};
					}
				}

				if (auto mem = load_file(fx_filepath); mem && !mem->empty())
				{
					ceffect_parser parser(mem->data());
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
			else
			{
				//- Load vertex shader
				{
					const auto filepath = filepath_t(snaps->m_vs.m_filepath_or_name);

					scompile_options options;
					options.m_name = filepath.filename().generic_string();
					options.m_type = scompile_options::shader_type_vertex;
					options.m_include_dirs.push_back(filepath.parent_path().generic_string());

					switch (snaps->m_vs.m_type)
					{
					case seffect_snapshot::type_file:
					{
						if (auto mem = load_file(filepath); mem && !mem->empty())
						{
							ceffect_parser parser(mem->data());
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
						return {};
					}
					default:
					case seffect_snapshot::type_none:
						return {};
					}
				}

				//- Load pixel shader
				{
					const auto filepath = filepath_t(snaps->m_ps.m_filepath_or_name);

					scompile_options options;
					options.m_name = filepath.filename().generic_string();
					options.m_type = scompile_options::shader_type_pixel;
					options.m_include_dirs.push_back(filepath.parent_path().generic_string());

					switch (snaps->m_ps.m_type)
					{
					case seffect_snapshot::type_file:
					{
						if (auto mem = load_file(filepath); mem && !mem->empty())
						{
							ceffect_parser parser(mem->data());
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
						return {};
					}
					default:
					case seffect_snapshot::type_none:
						return {};
					}
				}

				//- Finally, create the program from created shaders
				effect.m_program = bgfx::createProgram(effect.m_vs.m_handle, effect.m_ps.m_handle);
			}

			//- Load uniform data
			for (const auto& snapshot_uniform : snaps->m_uniforms)
			{
				auto& uniform = effect.m_uniforms.emplace_back();
				uniform = std::move(create_uniform(uniform.m_name.c_str(), uniform.m_type));
				if (uniform.m_data.is_valid())
				{
					auto data = snapshot_uniform.m_data;
					update_uniform(uniform, std::move(data));
				}
			}
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ceffect_resource_manager_service::do_destroy(seffect* inst)
	{
		//- Destroy the uniforms if effect is using any
		for (auto& uniform : inst->m_uniforms)
		{
			if (bgfx::isValid(uniform.m_handle))
			{
				bgfx::destroy(uniform.m_handle);
			}
		}

		//- Destroy the vertex and pixel shaders
		if (bgfx::isValid(inst->m_vs.m_handle))
		{
			bgfx::destroy(inst->m_vs.m_handle);
		}
		if (bgfx::isValid(inst->m_ps.m_handle))
		{
			bgfx::destroy(inst->m_ps.m_handle);
		}
		if (bgfx::isValid(inst->m_program))
		{
			bgfx::destroy(inst->m_program);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	suniform create_uniform(const char* name, suniform::type type)
	{
		suniform output;
		output.m_name = name;
		output.m_handle = bgfx::createUniform(name, type);

		if (bgfx::isValid(output.m_handle))
		{
			output.m_type = type;
			return output;
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	void update_uniform(suniform& uniform, rttr::variant&& data)
	{
		uniform.m_data = std::move(data);
		bgfx::setUniform(uniform.m_handle, uniform.m_data.get_raw_ptr());
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