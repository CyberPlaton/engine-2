#pragma once
#include <bgfx.h>
#include <rttr.h>
#include <string>
#include <vector>
#include <core/memory.hpp>
#include <engine/services/resource_manager_service.hpp>

namespace kokoro
{
	//- Serializable representation of an effect.
	//------------------------------------------------------------------------------------------------------------------------
	struct seffect_snapshot final
	{
		enum type : uint8_t
		{
			type_none = 0,
			type_file,
			type_embedded,
		};

		struct ssnapshot_shader
		{
			std::string m_filepath_or_name;
			type m_type = type_none;
		};

		ssnapshot_shader m_vs;
		ssnapshot_shader m_ps;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct seffect
	{
		static std::pair<bool, seffect>	load(const rttr::variant& snapshot);
		static void						unload(seffect& effect);

		struct sshader
		{
			bgfx::ShaderHandle m_handle = BGFX_INVALID_HANDLE;
		};

		sshader m_vs;
		sshader m_ps;
		bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
	};

} //- kokoro