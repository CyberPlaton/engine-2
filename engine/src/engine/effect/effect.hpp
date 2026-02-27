#pragma once
#include <bgfx.h>
#include <rttr.h>
#include <string>
#include <vector>
#include <core/memory.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	struct suniform final
	{
		using type = bgfx::UniformType::Enum;

		std::string m_name;
		rttr::variant m_data;
		type m_type = type::Count;
		bgfx::UniformHandle m_handle = BGFX_INVALID_HANDLE;
	};

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
		};

		sshader m_vs;
		sshader m_ps;
		std::vector<suniform> m_uniforms;
	};

	//- Runtime instance of an effect created from a snapshot. From the same snapshot we can have many effect instances
	//------------------------------------------------------------------------------------------------------------------------
	struct seffect final
	{
		struct sshader
		{
			core::memory_ref_t m_data = nullptr;
			bgfx::ShaderHandle m_handle = BGFX_INVALID_HANDLE;
		};

		sshader m_vs;
		sshader m_ps;
		std::vector<suniform> m_uniforms;
		bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
	};

	seffect_snapshot*	effect_snapshot_from_file(const char* filepath);
	seffect*			instantiate_effect(const seffect_snapshot& snapshot, const char* name);
	void				destroy_instance(const char* name);

} //- kokoro