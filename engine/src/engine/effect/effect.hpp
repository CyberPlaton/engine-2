#pragma once
#include <engine/services/resource_manager_service.hpp>
#include <core/memory.hpp>
#include <bgfx.h>
#include <rttr.h>
#include <string>
#include <vector>
#include <optional>

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

		struct sshader
		{
			std::string m_filepath_or_name;
			type m_type = type_none;
			RTTR_ENABLE();
		};

		sshader m_vs;
		sshader m_ps;
		RTTR_ENABLE();
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct seffect
	{
		static std::optional<seffect>	load(const rttr::variant& snapshot);
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