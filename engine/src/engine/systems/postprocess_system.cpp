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

			//- Name -> volume ptr for lookup
			std::unordered_map<std::string, const world::component::spostprocess_volume*> by_name;
			by_name.reserve(raw.size());
			for (const auto* vol : raw)
			{
				by_name[vol->m_postprocess.get().m_name] = vol;
			}

			//- Build adjacency list and in-degree from declared constraints.
			//- An edge A -> B means A must execute before B.
			//- predecessors of P => they point TO P  (they run before P)
			//- successors of P   => P points TO them (P runs before them)
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
					if (by_name.count(pred))
					{
						adj[pred].push_back(pp.m_name);
						in_degree[pp.m_name]++;
					}
				}

				for (const auto& succ : pp.m_successors)
				{
					if (by_name.count(succ))
					{
						adj[pp.m_name].push_back(succ);
						in_degree[succ]++;
					}
				}
			}

			//- Kahn's algorithm with a min-heap instead of a plain queue.
			//- When multiple nodes are unconstrained (in_degree == 0) simultaneously,
			//- they are processed in alphabetical order by name, giving a stable and
			//- deterministic result regardless of hash map iteration order or the order
			//- in which the multithreaded gather system populated raw_postprocesses.
			std::priority_queue<std::string, std::vector<std::string>, std::greater<std::string>> ready;

			for (const auto& [name, deg] : in_degree)
			{
				if (deg == 0) ready.push(name);
			}

			while (!ready.empty())
			{
				const auto name = ready.top();
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
		//- Ensure the post process and all the subpasses are ready
		if (!c.m_postprocess)
		{
			return;
		}

		for (const auto& subpass : c.m_postprocess.get().m_passes)
		{
			if (!subpass.m_effect)
			{
				return;
			}
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

		//- Ensure we are ready for executing post processes
		auto& rs = instance().service<crender_service>();

		if (!rs.merge_program(0) || !rs.merge_program(1))
		{
			raw_postprocesses.clear();
			ordered_postprocesses.clear();
			return;
		}

		ordered_postprocesses.clear();
		sort_postprocesses(raw_postprocesses, ordered_postprocesses);
		raw_postprocesses.clear();

		//- Given ordered postprocesses, execute them
		{
			auto geometry_framebuffer = rs.geometry_framebuffer();
			auto postprocess_counter = 0;

			for (auto i = 0; i < ordered_postprocesses.size(); ++i)
			{
				auto& postprocess = ordered_postprocesses[i]->m_postprocess.get();

				for (auto j = 0; j < postprocess.m_passes.size(); ++j)
				{
					auto& subpass = postprocess.m_passes[j];

					const auto view = bgfx::ViewId(C_POSTPROCESS_PASS_ID + postprocess_counter);

					//- Setup state for subpass
					bgfx::setMarker(fmt::format("{} Subpass #{}", postprocess.m_name, j).c_str());
					bgfx::setViewFrameBuffer(view, subpass.m_output_framebuffer);
					bgfx::setViewClear(view, BGFX_CLEAR_COLOR, 0xffffffff);
					bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
					bgfx::setViewRect(view, subpass.m_x, subpass.m_y, subpass.m_backbuffer_ratio);
					bgfx::setState(subpass.m_state | subpass.m_blending);

					//- Bind required framebuffer textures
					for (auto k = 0; k < subpass.m_framebuffer_inputs.size(); ++k)
					{
						const auto& input = subpass.m_framebuffer_inputs[k];

						bgfx::FrameBufferHandle input_framebuffer = BGFX_INVALID_HANDLE;
						if (input.m_input_index == spostprocess_subpass::C_CHAIN_FRAMEBUFFER)
						{
							//- Take either the in-out framebuffer if we are the first post process and no previous
							//- exists logically, or take the last output of the previous framebuffer
							input_framebuffer = i == 0 ? geometry_framebuffer :
								ordered_postprocesses[i - 1]->m_postprocess.get().m_passes.back().m_output_framebuffer;
						}
						else
						{
							//- Take as input the output of one of the subpasses of this postprocess
							input_framebuffer = postprocess.m_passes[input.m_input_index].m_output_framebuffer;
						}

						bgfx::setTexture(static_cast<uint8_t>(k), input.m_input_uniform.m_handle, bgfx::getTexture(input_framebuffer));
					}

					//- Bind global engine and pass defined uniforms
					rs.bind_builtin_uniforms();
					rs.submit_screen_quad();

					//- Finally execute the subpass
					bgfx::submit(view, subpass.m_effect.get().m_program);

					++postprocess_counter;
				}
			}

			//- Execute the last postprocess in the chain, effectively writing accumulated data to in_out framebuffer
			{
				const auto& last = ordered_postprocesses.back()->m_postprocess.get().m_passes.back();
				const auto view = bgfx::ViewId(C_POSTPROCESS_PASS_ID + postprocess_counter);

				bgfx::setMarker("postprocesses chain merge");
				bgfx::setViewFrameBuffer(view, geometry_framebuffer);
				bgfx::setViewClear(view, BGFX_CLEAR_COLOR, 0xffffffff);
				bgfx::setTexture(0, rs.chain_framebuffer_uniform(), bgfx::getTexture(last.m_output_framebuffer));
				bgfx::setViewRect(view, 0, 0, backbuffer_ratio_t::Equal);
				bgfx::setState(0
					| BGFX_STATE_WRITE_RGB
					| BGFX_STATE_WRITE_A
					| BGFX_STATE_CULL_CW);

				rs.submit_screen_quad();

				bgfx::submit(view, rs.merge_program(static_cast<uint16_t>(postprocess_counter)).get().m_program);
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