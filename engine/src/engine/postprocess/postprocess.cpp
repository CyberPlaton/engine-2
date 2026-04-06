#include <engine/postprocess/postprocess.hpp>
#include <engine/services/resource_manager_service.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/render_service.hpp>
#include <engine/effect/effect.hpp>
#include <core/registrator.hpp>
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
		postprocess.m_predecessors = snaps.m_predecessors;
		postprocess.m_successors = snaps.m_successors;
		postprocess.m_name = snaps.m_name;

		//- Load all of the defined subpasses
		for (const auto& snapshot_pass : snaps.m_passes)
		{
			auto& pass = postprocess.m_passes.emplace_back();

			//- Load the effect for the subpass
			pass.m_effect = rms.load<seffect>(snapshot_pass.m_effect);

			//- Load the textures to be used for binding
			for (const auto& snapshot_sampler : snapshot_pass.m_sampler_textures)
			{
				auto& tex = pass.m_sampler_textures.emplace_back();
				tex.m_texture = rms.load<stexture>(snapshot_sampler.m_texture);
				tex.m_sampler_stage = snapshot_sampler.m_sampler_stage;
				tex.m_sampler_flags = snapshot_sampler.m_sampler_flags;
			}

			//- Load the uniforms to be used for binding
			for (const auto& snapshot_uniform : snapshot_pass.m_uniforms)
			{
				auto& uniform = pass.m_uniforms.emplace_back();
				uniform = create_uniform(snapshot_uniform.m_name.c_str(), snapshot_uniform.m_type);
				if (snapshot_uniform.m_data.is_valid())
				{
					auto data = snapshot_uniform.m_data;
					update_uniform(uniform, std::move(data));
				}
			}

			//- Set common data for subpass and the output framebuffer and framebuffer sampler
			pass.m_blending = snapshot_pass.m_blending;
			pass.m_state = snapshot_pass.m_state;
			pass.m_backbuffer_ratio = snapshot_pass.m_backbuffer_ratio;
			pass.m_output_framebuffer = bgfx::createFrameBuffer(pass.m_backbuffer_ratio, texture_format_t::RGBA8,
				BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

			for (const auto& snapshot_input : snapshot_pass.m_framebuffer_inputs)
			{
				auto& input = pass.m_framebuffer_inputs.emplace_back();
				input.m_input_index = snapshot_input.m_input_index;
				input.m_input_uniform = create_uniform(snapshot_input.m_input_uniform.m_name.c_str(), uniform_type_t::Sampler);
			}
		}

		return std::move(postprocess);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void spostprocess::unload(spostprocess& postprocess)
	{

	}

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro;

	rttr::detail::default_constructor<std::vector<suniform>>();
	rttr::detail::default_constructor<std::vector<spostprocess_snapshot::sampler_texture_t>>();
	rttr::detail::default_constructor<std::vector<spostprocess_snapshot::ssubpass>>();

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<spostprocess_snapshot::ssubpass::sframebuffer_input>("spostprocess_snapshot::ssubpass::sframebuffer_input")
		.prop("m_input_uniform", &spostprocess_snapshot::ssubpass::sframebuffer_input::m_input_uniform)
		.prop("m_input_index", &spostprocess_snapshot::ssubpass::sframebuffer_input::m_input_index);

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<spostprocess_snapshot::ssubpass>("spostprocess_snapshot::ssubpass")
		.prop("m_uniforms", &spostprocess_snapshot::ssubpass::m_uniforms)
		.prop("m_sampler_textures", &spostprocess_snapshot::ssubpass::m_sampler_textures)
		.prop("m_framebuffer_inputs", &spostprocess_snapshot::ssubpass::m_framebuffer_inputs)
		.prop("m_effect", &spostprocess_snapshot::ssubpass::m_effect)
		.prop("m_blending", &spostprocess_snapshot::ssubpass::m_blending)
		.prop("m_state", &spostprocess_snapshot::ssubpass::m_state)
		.prop("m_backbuffer_ratio", &spostprocess_snapshot::ssubpass::m_backbuffer_ratio);

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<spostprocess_snapshot>("spostprocess_snapshot")
		.prop("m_predecessors", &spostprocess_snapshot::m_predecessors)
		.prop("m_successors", &spostprocess_snapshot::m_successors)
		.prop("m_passes", &spostprocess_snapshot::m_passes)
		.prop("m_name", &spostprocess_snapshot::m_name);

}