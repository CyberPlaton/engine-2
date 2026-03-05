#pragma once
#include <bgfx.h>
#include <rttr.h>
#include <string>
#include <vector>
#include <core/memory.hpp>
#include <engine/iresource_manager_service.hpp>

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

		struct ssnapshot_shader
		{
			std::string m_filepath_or_name;
			type m_type = type_none;
		};

		ssnapshot_shader m_vs;
		ssnapshot_shader m_ps;
		std::vector<suniform> m_uniforms;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct seffect
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

	//------------------------------------------------------------------------------------------------------------------------
	class ceffect_resource_manager_service final : public iresource_manager_service<seffect, seffect_snapshot>
	{
	public:
		ceffect_resource_manager_service() = default;
		~ceffect_resource_manager_service() = default;

	protected:
		seffect			do_instantiate(const seffect_snapshot* snaps) override final;
		void			do_destroy(seffect* inst) override final;
	};

	suniform			create_uniform(const char* name, suniform::type type);
	void				update_uniform(suniform& uniform, rttr::variant&& data);

} //- kokoro