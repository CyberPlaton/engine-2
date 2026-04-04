#include <engine/postprocess/postprocess.hpp>
#include <engine/services/resource_manager_service.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/render_service.hpp>
#include <engine/effect/effect.hpp>
#include <engine.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	std::optional<spostprocess> spostprocess::load(const rttr::variant& snapshot)
	{
		if (!snapshot.is_valid())
		{
			instance().service<clog_service>().err("spostprocess::load - received invalid snapshot variant");
			return std::nullopt;
		}

		const auto& snaps = snapshot.get_value<spostprocess_snapshot>();
		auto& rms = instance().service<cresource_manager_service>();
		auto& log = instance().service<clog_service>();
		spostprocess postprocess;

		//- Load the effect to be used
		postprocess.m_effect = rms.load<seffect>(snaps.m_effect);

		//- Load the textures to be used for binding
		for (const auto& sampler : snaps.m_sampler_textures)
		{
			auto& tex = postprocess.m_sampler_textures.emplace_back();
			tex.m_texture = rms.load<stexture>(sampler.m_texture);
			tex.m_sampler_stage = sampler.m_sampler_stage;
			tex.m_sampler_flags = sampler.m_sampler_flags;
		}

		//- Load the uniforms to be used for binding
		for (const auto& uniform : snaps.m_uniforms)
		{
			auto& uni = postprocess.m_uniforms.emplace_back();
			uni = create_uniform(uniform.m_name.c_str(), uniform.m_type);
			if (uniform.m_data.is_valid())
			{
				auto data = uniform.m_data;
				update_uniform(uni, std::move(data));
			}
		}

		//- Set common data for post process
		postprocess.m_predecessors = snaps.m_predecessors;
		postprocess.m_successors = snaps.m_successors;
		postprocess.m_name = snaps.m_name;
		postprocess.m_blending = snaps.m_blending;
		postprocess.m_state = snaps.m_state;
		postprocess.m_backbuffer_ratio = snaps.m_backbuffer_ratio;

		//- Set the output framebuffer and framebuffer sampler
		postprocess.m_output_framebuffer = bgfx::createFrameBuffer(postprocess.m_backbuffer_ratio, texture_format_t::RGBA8,
			BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
		postprocess.m_framebuffer_sampler = create_uniform("s_texture", uniform_type_t::Sampler);

		return std::move(postprocess);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void spostprocess::unload(spostprocess& postprocess)
	{

	}

} //- kokoro