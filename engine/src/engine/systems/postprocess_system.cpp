#include <engine/systems/postprocess_system.hpp>
#include <engine/services/render_service.hpp>
#include <engine.hpp>
#include <core/mutex.hpp>

namespace kokoro
{
	namespace
	{
		core::cmutex mutex;
		std::vector<const world::component::spostprocess_volume*> raw_postprocesses;
		std::vector<const world::component::spostprocess_volume*> ordered_postprocesses;

		//------------------------------------------------------------------------------------------------------------------------
		void sort_postprocesses(const std::vector<const world::component::spostprocess_volume*>& raw,
			std::vector<const world::component::spostprocess_volume*>& out)
		{
			if (raw.empty()) return;

			// Name -> volume ptr for lookup
			std::unordered_map<std::string, const world::component::spostprocess_volume*> by_name;
			by_name.reserve(raw.size());

			for (const auto* vol : raw)
			{
				by_name[vol->m_postprocess.get().m_name] = vol;
			}

			// Build adjacency list and in-degree from declared constraints.
			// An edge A -> B means A must execute before B.
			// predecessors of P => they point TO P  (they run before P)
			// successors of P   => P points TO them (P runs before them)
			std::unordered_map<std::string, std::vector<std::string>> adj;
			std::unordered_map<std::string, int> in_degree;

			for (const auto* vol : raw)
			{
				const auto& pp = vol->m_postprocess.get();
				in_degree.emplace(pp.m_name, 0);
				adj.emplace(pp.m_name, std::vector<std::string>{});
			}

			for (const auto* vol : raw)
			{
				const auto& pp = vol->m_postprocess.get();

				for (const auto& pred : pp.m_predecessors)
				{
					// pred -> pp.m_name
					if (by_name.count(pred))
					{
						adj[pred].push_back(pp.m_name);
						in_degree[pp.m_name]++;
					}
				}

				for (const auto& succ : pp.m_successors)
				{
					// pp.m_name -> succ
					if (by_name.count(succ))
					{
						adj[pp.m_name].push_back(succ);
						in_degree[succ]++;
					}
				}
			}

			// Kahn's algorithm
			std::queue<std::string> ready;

			for (const auto& [name, deg] : in_degree)
			{
				if (deg == 0) ready.push(name);
			}

			while (!ready.empty())
			{
				const auto name = ready.front();
				ready.pop();

				out.push_back(by_name.at(name));

				for (const auto& next : adj.at(name))
				{
					if (--in_degree[next] == 0)
						ready.push(next);
				}
			}

			if (out.size() != raw.size())
			{
				//- Report that we have detected a cycle
			}
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	spostprocess_submit_system::spostprocess_submit_system(kokoro::cworld* w)
	{
		w->system_manager().create_task(config(), postprocess_submit_system);
	}

	//------------------------------------------------------------------------------------------------------------------------
	spostprocess_gather_system::spostprocess_gather_system(kokoro::cworld* w)
	{
		w->system_manager().create_system(config(), postprocess_gather_system);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void postprocess_gather_system(flecs::entity e, const world::component::spostprocess_volume& c)
	{
		if (!c.m_postprocess.ready() || !c.m_postprocess.get().m_effect.ready())
		{
			return;
		}

		core::cscoped_mutex m(mutex);
		raw_postprocesses.push_back(&c);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void postprocess_submit_system(flecs::iter& it, float dt)
	{
		//- Given raw submission order, create an actual order of execution
		if (raw_postprocesses.empty())
		{
			return;
		}
		ordered_postprocesses.clear();
		sort_postprocesses(raw_postprocesses, ordered_postprocesses);
		raw_postprocesses.clear();

		//- Given ordered postprocesses, execute them
		{
			auto& rs = instance().service<crender_service>();
			auto in_out = rs.geometry_framebuffer();

			//- Execute the initial postprocess in the chain
			{
				const auto& first = ordered_postprocesses[0]->m_postprocess.get();

				bgfx::setMarker(first.m_name.c_str());
				bgfx::setViewFrameBuffer(C_POSTPROCESS_PASS_ID + 0, first.m_output_framebuffer);
				bgfx::setViewClear(C_POSTPROCESS_PASS_ID + 0, BGFX_CLEAR_COLOR, 0xffffffff);
				bgfx::setTexture(0, first.m_framebuffer_sampler.m_handle, bgfx::getTexture(in_out));
				bgfx::setViewRect(C_POSTPROCESS_PASS_ID + 0, first.m_x, first.m_y, first.m_backbuffer_ratio);
				bgfx::setState(first.m_state | first.m_blending);

				rs.bind_builtin_uniforms();
				rs.submit_screen_quad();
				bgfx::submit(C_POSTPROCESS_PASS_ID + 0, first.m_effect.get().m_program);
			}

			//- Execute the chain of postprocesses up to the last, where i + 1 takes the output of i and works with that
			{
				for (auto i = 1; i < ordered_postprocesses.size(); ++i)
				{
					const auto& previous = ordered_postprocesses[i - 1]->m_postprocess.get();
					const auto& pp = ordered_postprocesses[i]->m_postprocess.get();
					const auto view = bgfx::ViewId(C_POSTPROCESS_PASS_ID + i);

					bgfx::setMarker(pp.m_name.c_str());
					bgfx::setViewFrameBuffer(view, pp.m_output_framebuffer);
					bgfx::setViewClear(view, BGFX_CLEAR_COLOR, 0xffffffff);
					bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
					bgfx::setTexture(0, pp.m_framebuffer_sampler.m_handle, bgfx::getTexture(previous.m_output_framebuffer));
					bgfx::setViewRect(view, pp.m_x, pp.m_y, pp.m_backbuffer_ratio);
					bgfx::setState(pp.m_state | pp.m_blending);

					rs.bind_builtin_uniforms();
					rs.submit_screen_quad();

					bgfx::submit(view, pp.m_effect.get().m_program);
				}
			}

			//- Execute the last postprocess in the chain, effectively writing accumulated data to in_out framebuffer
			{
				const auto& last = ordered_postprocesses.back()->m_postprocess.get();
				const auto view = bgfx::ViewId(C_POSTPROCESS_PASS_ID + ordered_postprocesses.size());

				bgfx::setMarker("postprocesses chain merge");
				bgfx::setViewFrameBuffer(view, in_out);
				bgfx::setViewClear(view, BGFX_CLEAR_COLOR, 0xffffffff);
				bgfx::setTexture(0, last.m_framebuffer_sampler.m_handle, bgfx::getTexture(last.m_output_framebuffer));
				bgfx::setViewRect(view, 0, 0, backbuffer_ratio_t::Equal);
				bgfx::setState(0
					| BGFX_STATE_WRITE_RGB
					| BGFX_STATE_WRITE_A
					| BGFX_STATE_CULL_CW);

				rs.bind_builtin_uniforms();
				rs.submit_screen_quad();

				bgfx::submit(view, rs.merge_program(static_cast<uint16_t>(ordered_postprocesses.size())).get().m_program);
			}
		}
	}

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro;

	REGISTER_SYSTEM(spostprocess_gather_system);
	REGISTER_SYSTEM(spostprocess_submit_system);
}