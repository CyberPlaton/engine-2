#include <engine/effect/shader.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <engine.hpp>
#include <core/mutex.hpp>
#include <../bgfx/tools/shaderc/shaderc.h>
#include <fmt.h>

namespace kokoro
{
	namespace
	{
		core::cmutex mutex;

		//------------------------------------------------------------------------------------------------------------------------
		template<typename TIterator>
		std::string join(TIterator begin, TIterator end, const char* delim)
		{
			return fmt::to_string(fmt::join(begin, end, delim));
		}

		//- Write incoming data to a string form
		//------------------------------------------------------------------------------------------------------------------------
		class cstringwriter final : public bx::WriterI
		{
		public:
			int32_t write(const void* data, int32_t size, bx::Error* error) override final
			{
				if (error->isOk())
				{
					const char* text = (const char*)data;
					std::string string(text, size);
					m_string += string;
					return size;
				}
				return -1;
			}

			std::string_view view() const { return m_string; }

		private:
			std::string m_string;
		};

		//- Write incoming data to log. Convenience class to use with thirdparty libraries,
		//- such as bx, bgfx etc
		//------------------------------------------------------------------------------------------------------------------------
		class clogwriter final : public bx::WriterI
		{
		public:
			clogwriter() = default;
			~clogwriter() = default;

			int32_t write(const void* data, int32_t size, bx::Error* error) override final
			{
				if (size == 1)
				{
					m_buffer.push_back(*(char*)data);
				}
				else
				{
					bx::StringView view((const char*)data, size);
					m_buffer.insert(m_buffer.end(), view.getPtr(), view.getPtr() + view.getLength());
				}
				return size;
			}

			std::string m_buffer;
		};

		//------------------------------------------------------------------------------------------------------------------------
		bgfx::Options to_bgfx_options(const scompile_options& options)
		{
			bgfx::Options out;

			switch (options.m_type)
			{
			default:
			case scompile_options::shader_type_none: return {};
			case scompile_options::shader_type_vertex:
			{
				out.shaderType = 'v';
				break;
			}
			case scompile_options::shader_type_pixel:
			{
				out.shaderType = 'f';
				break;
			}
			case scompile_options::shader_type_compute:
			{
				out.shaderType = 'c';
				break;
			}
			}

			out.platform = options.m_platform;
			out.profile = options.m_profile;
			out.debugInformation = !!(options.m_flags & scompile_options::flag_debug);
			out.avoidFlowControl = !!(options.m_flags & scompile_options::flag_avoid_flow_control);
			out.noPreshader = !!(options.m_flags & scompile_options::flag_no_preshader);
			out.partialPrecision = !!(options.m_flags & scompile_options::flag_partial_precision);
			out.preferFlowControl = !!(options.m_flags & scompile_options::flag_prefer_flow_control);
			out.backwardsCompatibility = !!(options.m_flags & scompile_options::flag_backward_compatible);
			out.warningsAreErrors = !!(options.m_flags & scompile_options::flag_warnings_are_errors);
			out.keepIntermediate = !!(options.m_flags & scompile_options::flag_keep_intermediate);
			out.optimize = options.m_optimization != scompile_options::optimization_level_none;
			out.optimizationLevel = static_cast<unsigned>(options.m_optimization);

			for (const auto& include : options.m_include_dirs)
			{
				out.includeDirs.emplace_back(include.data());
			}
			for (const auto& define : options.m_defines)
			{
				out.defines.emplace_back(define.data());
			}
			for (const auto& dep : options.m_deps)
			{
				out.dependencies.emplace_back(dep.data());
			}
			return out;
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	std::string scompile_options::shader_profile()
	{
#if PLATFORM_WINDOWS
		static constexpr const char* C_PROFILE = "s_5_0";
#elif PLATFORM_LINUX || PLATFORM_MACOSX
		static constexpr const char* C_PROFILE = "spirv";
#endif
		return C_PROFILE;
	}

	//------------------------------------------------------------------------------------------------------------------------
	std::string scompile_options::shader_varying_default()
	{
		static constexpr const char* C_VARYING =
			"vec3 a_position  : POSITION;\n"
			"vec4 a_color0    : COLOR0;\n"
			"vec2 a_texcoord0 : TEXCOORD0;\n"

			"vec4 v_color     : COLOR        = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"vec2 v_texcoord0 : TEXCOORD0    = vec2(0.0, 0.0);\n"

			"\n\0";

		return C_VARYING;
	}

	//------------------------------------------------------------------------------------------------------------------------
	std::string scompile_options::shader_platform()
	{
#if PLATFORM_WINDOWS
		static constexpr const char* C_PLATFORM = "windows";
#elif PLATFORM_LINUX
		static constexpr const char* C_PLATFORM = "linux";
#elif PLATFORM_MACOSX
		static constexpr const char* C_PLATFORM = "osx";
#endif
		return C_PLATFORM;
	}

	//------------------------------------------------------------------------------------------------------------------------
	core::memory_ref_t compile_shader_from_string(const char* code, scompile_options& options)
	{
		core::cscoped_mutex m(mutex);

		auto& vfs = instance().service<cvirtual_filesystem_service>();
		auto& log = instance().service<clog_service>();

		//- In case we did not set the paths previously, we have to set them here
		{
			options.m_include_dirs.push_back(vfs.basepath("engine"));
			options.m_include_dirs.push_back(vfs.basepath("engine") + "/shaders");
			options.m_include_dirs.push_back(vfs.basepath("/"));
		}

		//- Extended compile information
		{
			log.debug(fmt::format("-------------------------------- Compiling shader '{}' ------------------------------------",
				options.m_name).c_str());
			
			log.debug(fmt::format("\tplatform '{}'",
				options.m_platform).c_str());

			log.debug(fmt::format("\tprofile '{}'",
				options.m_profile).c_str());

			log.debug(fmt::format("\ttype '{}'",
				options.m_type == scompile_options::shader_type_vertex ? "Vertex" :
				options.m_type == scompile_options::shader_type_pixel ? "Pixel" :
				options.m_type == scompile_options::shader_type_compute ? "Compute" : "-/-").c_str());

			log.debug(fmt::format("\toptimization '{} ({})'",
				static_cast<unsigned>(options.m_optimization),
				options.m_optimization != scompile_options::optimization_level_none ? "Enabled" : "Disabled").c_str());

			log.debug(fmt::format("\tvarying:\n'{}'",
				options.m_varying).c_str());

			log.debug(fmt::format("\tincludes '{}'",
				join(options.m_include_dirs.begin(), options.m_include_dirs.end(), ",  ")).c_str());

			log.debug(fmt::format("\tdefines '{}'",
				join(options.m_defines.begin(), options.m_defines.end(), ",  ")).c_str());

			log.debug(fmt::format("\tdeps '{}'",
				join(options.m_deps.begin(), options.m_deps.end(), ",  ")).c_str());
		}

		auto bgfx_options = to_bgfx_options(options);
		filepath_t temp_path;

		//- Save code to a temporary file for compilation as direct compiling from string is not supported
		{
			temp_path = fmt::format("{}/{}.sc",
				vfs.basepath(".temp"),
				filepath_t(options.m_name).stem().generic_string());

			//- Recreate the file and write shader data to it
			if(auto wrapper = vfs.open(temp_path, file_options_write | file_options_text | file_options_truncate); wrapper)
			{
				auto& file = wrapper.get();
				file.write(code, static_cast<unsigned>(strlen(code)));
				file.close();

				log.debug(fmt::format("\tsucessfully created and wrote to temporary file '{}'",
					temp_path.generic_string()).c_str());
			}
			else
			{
				log.err(fmt::format("\tfailed creating and writing to temporary file '{}'",
					temp_path.generic_string()).c_str());
				return nullptr;
			}
		}

		clogwriter logwriter;
		cstringwriter stringwriter;
		core::memory_ref_t memory{};
		bgfx_options.inputFilePath = temp_path.generic_string();

		if (bgfx::compileShader(options.m_varying.data(), nullptr, (char*)code, static_cast<unsigned>(strlen(code)),
			false, bgfx_options, &stringwriter, &logwriter))
		{
			memory = std::make_shared<core::cmemory>((char*)stringwriter.view().data(), stringwriter.view().size());

			log.info("\tsucessfully compiled");
		}
		else
		{
			log.err("\tfailed to compile:\n'{}'",
				logwriter.m_buffer);
		}

		//- Erase the temporary file used for compilation
		{
			std::error_code err;
			std::filesystem::remove_all(temp_path, err);
			vfs.evict(temp_path);
		}

		return memory;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bgfx::ProgramHandle create_compute_program(core::memory_ref_t cs)
	{
		const auto compute = bgfx::createShader(bgfx::makeRef(cs->data(), cs->size()));

		if (bgfx::isValid(compute))
		{
			return bgfx::createProgram(compute);
		}
		return { bgfx::kInvalidHandle };
	}

	//------------------------------------------------------------------------------------------------------------------------
	bgfx::ProgramHandle create_program(core::memory_ref_t vs, core::memory_ref_t ps)
	{
		const auto vertex = bgfx::createShader(bgfx::makeRef(vs->data(), vs->size()));
		const auto pixel = bgfx::createShader(bgfx::makeRef(ps->data(), ps->size()));

		if (bgfx::isValid(vertex) && bgfx::isValid(pixel))
		{
			if (const auto handle = bgfx::createProgram(vertex, pixel); bgfx::isValid(handle))
			{
				return handle;
			}
		}
		return { bgfx::kInvalidHandle };
	}

} //- kokoro