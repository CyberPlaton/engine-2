#include <engine/material/material.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/io.hpp>
#include <core/mutex.hpp>
#include <engine.hpp>
#include <fmt.h>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	bool cmaterial_resource_manager_service::init()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	smaterial cmaterial_resource_manager_service::do_instantiate(const smaterial_snapshot* snaps)
	{
		auto& tms = instance().service<ctexture_resource_manager_service>();
		auto& ems = instance().service<ceffect_resource_manager_service>();
		smaterial material;

		//- Try loading the effect
		material.m_effect = ems.instantiate(snaps->m_effect);
		material.m_blending = snaps->m_blending;
		material.m_state = snaps->m_state;

		//- Try loading the textures
		for (auto i = 0; i < snaps->m_sampler_textures.size(); ++i)
		{
			auto& sampler = material.m_sampler_textures.emplace_back();
			const auto& t = snaps->m_sampler_textures[i];
			sampler.m_texture = tms.instantiate(t.m_texture);
			sampler.m_sampler_flags = t.m_sampler_flags;
			sampler.m_sampler_stage = t.m_sampler_stage;
		}
		return material;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cmaterial_resource_manager_service::do_destroy(smaterial* inst)
	{
		auto& tms = instance().service<ctexture_resource_manager_service>();
		auto& ems = instance().service<ceffect_resource_manager_service>();

		ems.destroy(inst->m_effect);

		for (auto i = 0; i < inst->m_sampler_textures.size(); ++i)
		{
			auto& t = inst->m_sampler_textures[i];
			tms.destroy(const_cast<stexture*>(t.m_texture));
		}
	}

} //- kokoro