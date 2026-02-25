#pragma once
#include <bgfx.h>
#include <rttr.h>
#include <string>
#include <string_view>
#include <vector>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	struct suniform final
	{
		using type = bgfx::UniformType::Enum;

		std::string m_name;
		rttr::variant m_data;
		type m_type;
		bgfx::UniformHandle m_handle = BGFX_INVALID_HANDLE;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct seffect final
	{
		struct sshader
		{
			char* m_data = nullptr;
			uint64_t m_size = 0;
			bgfx::ShaderHandle m_handle = BGFX_INVALID_HANDLE;
		};

		sshader m_vs;
		sshader m_ps;
		std::vector<suniform> m_uniforms;
		bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
	};

	seffect*	create_from_file(std::string_view filepath);
	seffect*	create_from_memory(std::string_view name, const char* data, uint64_t size);
	seffect*	create_from_string(std::string_view name, std::string_view code) { return create_from_memory(name, code.data(), code.length()); }
	void		destroy(std::string_view filepath_or_name);

} //- kokoro