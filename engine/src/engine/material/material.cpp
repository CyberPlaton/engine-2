#include <engine/material/material.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/io.hpp>
#include <core/mutex.hpp>
#include <core/registrator.hpp>
#include <engine.hpp>
#include <fmt.h>

namespace kokoro
{
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

	//------------------------------------------------------------------------------------------------------------------------
	std::pair<bool, smaterial> smaterial::load(const rttr::variant& snapshot)
	{
		const auto& snaps = snapshot.get_value<smaterial_snapshot>();
		auto& rms = instance().service<cresource_manager_service>();
		smaterial material;

		//- Try loading the effect
		material.m_effect = rms.load<seffect>(snaps.m_effect);
		material.m_blending = snaps.m_blending;
		material.m_state = snaps.m_state;

		//- Try loading the textures
		for (auto i = 0; i < snaps.m_sampler_textures.size(); ++i)
		{
			auto& sampler = material.m_sampler_textures.emplace_back();
			const auto& t = snaps.m_sampler_textures[i];
			sampler.m_texture = rms.load<stexture>(t.m_texture);
			sampler.m_sampler_flags = t.m_sampler_flags;
			sampler.m_sampler_stage = t.m_sampler_stage;
		}

		//- Load uniform data
		for (const auto& snapshot_uniform : snaps.m_uniforms)
		{
			auto& uniform = material.m_uniforms.emplace_back();
			uniform = std::move(create_uniform(uniform.m_name.c_str(), uniform.m_type));
			if (uniform.m_data.is_valid())
			{
				auto data = snapshot_uniform.m_data;
				update_uniform(uniform, std::move(data));
			}
		}

		return { true, material };
	}

	//------------------------------------------------------------------------------------------------------------------------
	void smaterial::unload(smaterial& material)
	{
		auto& rms = instance().service<cresource_manager_service>();

		rms.unload<seffect>(material.m_effect.path());

		for (auto i = 0; i < material.m_sampler_textures.size(); ++i)
		{
			auto& t = material.m_sampler_textures[i];
			rms.unload<stexture>(t.m_texture.path());
		}

		//- Destroy the uniforms the material was using
		for (auto& uniform : material.m_uniforms)
		{
			if (bgfx::isValid(uniform.m_handle))
			{
				bgfx::destroy(uniform.m_handle);
			}
		}
	}

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro;

	rttr::detail::default_constructor<std::vector<suniform>>();
	rttr::detail::default_constructor<std::vector<smaterial_snapshot::ssampler_texture>>();

	rttr::cregistrator<suniform>("suniform")
		.prop("m_name", &suniform::m_name)
		.prop("m_data", &suniform::m_data)
		.prop("m_type", &suniform::m_type);

	rttr::cregistrator<smaterial_snapshot::ssampler_texture>("smaterial_snapshot::ssampler_texture")
		.prop("m_sampler_stage", &smaterial_snapshot::ssampler_texture::m_sampler_stage)
		.prop("m_sampler_flags", &smaterial_snapshot::ssampler_texture::m_sampler_flags)
		.prop("m_texture", &smaterial_snapshot::ssampler_texture::m_texture);

	rttr::cregistrator<smaterial_snapshot>("smaterial_snapshot")
		.prop("m_state", &smaterial_snapshot::m_state)
		.prop("m_blending", &smaterial_snapshot::m_blending)
		.prop("m_effect", &smaterial_snapshot::m_effect)
		.prop("m_ps", &smaterial_snapshot::m_sampler_textures)
		.prop("m_uniforms", &smaterial_snapshot::m_uniforms);

}