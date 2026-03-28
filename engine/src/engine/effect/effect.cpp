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

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	std::pair<bool, seffect> seffect::load(const rttr::variant& snapshot)
	{
		if (!snapshot.is_valid())
		{
			instance().service<clog_service>().err("seffect::load - received invalid snapshot variant");
			return { false, {} };
		}

		const auto& snaps = snapshot.get_value<seffect_snapshot>();
		auto& vfs = instance().service<cvirtual_filesystem_service>();
		auto& log = instance().service<clog_service>();

		//- Reads a file through the VFS and returns its contents, or nullptr on failure.
		const auto load_file = [&](const filepath_t& path) -> core::memory_ref_t
			{
				if (auto file = vfs.open(path, file_options_read | file_options_text); file.opened())
				{
					auto mem = file.read_sync();
					if (mem && !mem->empty())
					{
						return mem;
					}
				}
				log.err(fmt::format("seffect::load - failed to read file '{}'",
					path.generic_string()).c_str());
				return nullptr;
			};

		//- Compiles a shader and returns bytecode, or nullptr on failure.
		const auto compile = [&](const std::string& code, scompile_options& options) -> core::memory_ref_t
			{
				auto mem = compile_shader_from_string(code.c_str(), options);
				if (!mem)
				{
					log.err(fmt::format("seffect::load - compilation failed for shader '{}'",
						options.m_name).c_str());
				}
				return mem;
			};

		//- Phase 1: compile both shaders to memory.
		//- No bgfx handles are created here — if either compilation fails we bail cleanly.
		core::memory_ref_t vs_mem = nullptr;
		core::memory_ref_t ps_mem = nullptr;

		if (snaps.m_vs.m_filepath_or_name == snaps.m_ps.m_filepath_or_name)
		{
			//- Combined .fx file containing both vertex and pixel shader
			filepath_t fx_filepath = snaps.m_vs.m_filepath_or_name;

			if (!vfs.exists(fx_filepath))
			{
				if (const auto [result, p] = vfs.resolve(fx_filepath); result)
				{
					fx_filepath = p;
				}
				else
				{
					log.err(fmt::format("seffect::load - could not find fx file '{}'",
						snaps.m_vs.m_filepath_or_name).c_str());
					return { false, {} };
				}
			}

			auto file_mem = load_file(fx_filepath);
			if (!file_mem)
			{
				return { false, {} };
			}

			ceffect_parser parser(file_mem->data());
			const auto output = parser.parse();

			if (output.m_vs.empty() || output.m_ps.empty())
			{
				log.err(fmt::format("seffect::load - parser produced empty output for '{}'",
					fx_filepath.generic_string()).c_str());
				return { false, {} };
			}

			{
				scompile_options options;
				options.m_name = fx_filepath.filename().generic_string();
				options.m_type = scompile_options::shader_type_vertex;
				options.m_include_dirs.push_back(fx_filepath.parent_path().generic_string());
				vs_mem = compile(output.m_vs, options);
			}
			{
				scompile_options options;
				options.m_name = fx_filepath.filename().generic_string();
				options.m_type = scompile_options::shader_type_pixel;
				options.m_include_dirs.push_back(fx_filepath.parent_path().generic_string());
				ps_mem = compile(output.m_ps, options);
			}
		}
		else
		{
			//- Separate vertex and pixel shader sources (file or embedded)
			const auto compile_shader = [&](const seffect_snapshot::sshader& snap,
				scompile_options::shader_type type) -> core::memory_ref_t
				{
					const filepath_t filepath = snap.m_filepath_or_name;

					scompile_options options;
					options.m_name = filepath.filename().generic_string();
					options.m_type = type;
					options.m_include_dirs.push_back(filepath.parent_path().generic_string());

					switch (snap.m_type)
					{
					case seffect_snapshot::type_file:
					{
						auto file_mem = load_file(filepath);
						if (!file_mem) return nullptr;

						ceffect_parser parser(file_mem->data());
						const auto output = parser.parse();

						const auto& code = (type == scompile_options::shader_type_vertex)
							? output.m_vs : output.m_ps;

						if (code.empty())
						{
							log.err(fmt::format("seffect::load - parser produced empty {} output for '{}'",
								(type == scompile_options::shader_type_vertex ? "vertex" : "pixel"),
								filepath.generic_string()).c_str());
							return nullptr;
						}

						return compile(code, options);
					}

					case seffect_snapshot::type_embedded:
					{
						const char* code_ptr = sembedded_shaders::get(snap.m_filepath_or_name.c_str());
						if (!code_ptr)
						{
							log.err(fmt::format("seffect::load - embedded shader '{}' not found",
								snap.m_filepath_or_name).c_str());
							return nullptr;
						}

						ceffect_parser parser(code_ptr);
						const auto output = parser.parse();

						const auto& code = (type == scompile_options::shader_type_vertex)
							? output.m_vs : output.m_ps;

						if (code.empty())
						{
							log.err(fmt::format("seffect::load - parser produced empty {} output for embedded '{}'",
								(type == scompile_options::shader_type_vertex ? "vertex" : "pixel"),
								snap.m_filepath_or_name).c_str());
							return nullptr;
						}

						return compile(code, options);
					}

					default:
					case seffect_snapshot::type_none:
						log.err("seffect::load - shader snapshot has type_none");
						return nullptr;
					}
				};

			vs_mem = compile_shader(snaps.m_vs, scompile_options::shader_type_vertex);
			ps_mem = compile_shader(snaps.m_ps, scompile_options::shader_type_pixel);
		}

		//- Validate both compilations succeeded before touching bgfx
		if (!vs_mem || !ps_mem)
		{
			return { false, {} };
		}

		//- Phase 2: create all bgfx objects together.
		//- All three calls are made in immediate succession with no code between them
		//- that could fail, so the effect is never left in a half-constructed state.
		seffect effect;
		effect.m_vs.m_handle = bgfx::createShader(bgfx::copy(vs_mem->data(), vs_mem->size()));
		effect.m_ps.m_handle = bgfx::createShader(bgfx::copy(ps_mem->data(), ps_mem->size()));

		if (!bgfx::isValid(effect.m_vs.m_handle) || !bgfx::isValid(effect.m_ps.m_handle))
		{
			log.err("seffect::load - bgfx::createShader returned invalid handle");
			seffect::unload(effect);
			return { false, {} };
		}

		effect.m_program = bgfx::createProgram(effect.m_vs.m_handle, effect.m_ps.m_handle);

		if (!bgfx::isValid(effect.m_program))
		{
			log.err("seffect::load - bgfx::createProgram returned invalid handle");
			seffect::unload(effect);
			return { false, {} };
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
	rttr::cregistrator<seffect_snapshot::sshader>("seffect_snapshot::sshader")
		.prop("m_filepath_or_name", &seffect_snapshot::sshader::m_filepath_or_name)
		.prop("m_type", &seffect_snapshot::sshader::m_type);

	rttr::cregistrator<seffect_snapshot>("seffect_snapshot")
		.prop("m_vs", &seffect_snapshot::m_vs)
		.prop("m_ps", &seffect_snapshot::m_ps);

}