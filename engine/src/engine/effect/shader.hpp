#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <bgfx.h>
#include <core/memory.hpp>

namespace kokoro
{
	using option_flags_t = int;

	//- Structure containing options for shader compilation.
	//------------------------------------------------------------------------------------------------------------------------
	struct scompile_options final
	{
		enum shader_type : uint8_t
		{
			shader_type_none = 0,
			shader_type_vertex,
			shader_type_pixel,
			shader_type_compute,
		};

		enum optimization_level : uint8_t
		{
			optimization_level_none = 0,
			optimization_level_1,
			optimization_level_2,
			optimization_level_3,
		};

		enum flag : uint16_t
		{
			flag_avoid_flow_control		= 1 << 0,
			flag_no_preshader			= 1 << 1,
			flag_partial_precision		= 1 << 2,
			flag_prefer_flow_control	= 1 << 3,
			flag_backward_compatible	= 1 << 4,
			flag_warnings_are_errors	= 1 << 5,
			flag_keep_intermediate		= 1 << 6,
			flag_debug					= 1 << 7
		};

		static std::string shader_profile();
		static std::string shader_varying_default();
		static std::string shader_platform();

		std::vector<std::string> m_include_dirs;
		std::vector<std::string> m_defines;
		std::vector<std::string> m_deps;
		std::string m_name;
		std::string m_varying = shader_varying_default();
		std::string m_platform = shader_platform();
		std::string m_profile = shader_profile();
		shader_type m_type = shader_type_none;
		optimization_level m_optimization = optimization_level_none;
		option_flags_t m_flags = 0;
	};

	core::memory_ref_t	compile_shader_from_string(const char* code, const scompile_options& options);
	core::memory_ref_t	compile_shader_from_string(const char* code, const char* name, scompile_options::shader_type type);
	bgfx::ProgramHandle	create_compute_program(core::memory_ref_t cs);
	bgfx::ProgramHandle	create_program(core::memory_ref_t vs, core::memory_ref_t ps);

} //- kokoro