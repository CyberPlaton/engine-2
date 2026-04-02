#pragma once
#include <engine/world/system.hpp>
#include <engine/world/world.hpp>
#include <engine/components/postprocess_volume.hpp>

namespace kokoro
{
	void postprocess_submit_system(flecs::iter& it, float dt);
	void postprocess_gather_system(flecs::entity e, const world::component::spostprocess_volume& c);

	//------------------------------------------------------------------------------------------------------------------------
	struct spostprocess_submit_system final
	{
		spostprocess_submit_system() = default;
		spostprocess_submit_system(kokoro::cworld* w);

		static kokoro::system::sconfig config()
		{
			kokoro::system::sconfig cfg;

			cfg.m_name = "spostprocess_submit_system";
			cfg.m_run_after = { "spostprocess_gather_system" };
			cfg.m_run_before = {};
			cfg.m_flags = 0;

			return cfg;
		}

		RTTR_ENABLE()
	};

	//- Given post process volumes in the world, this system gathers those that affect the current iamge and create a raw submission
	//- structure. From that an ordered submission structure is made. This ordered structure is executed as a chain and represents the final image.
	//------------------------------------------------------------------------------------------------------------------------
	struct spostprocess_gather_system final
	{
		spostprocess_gather_system() = default;
		spostprocess_gather_system(kokoro::cworld* w);

		static kokoro::system::sconfig config()
		{
			kokoro::system::sconfig cfg;

			cfg.m_name = "spostprocess_gather_system";
			cfg.m_run_after = {};
			cfg.m_run_before = {};
			cfg.m_flags = kokoro::system::system_flag_multithreaded;

			return cfg;
		}

		RTTR_ENABLE()
	};

} //- kokoro