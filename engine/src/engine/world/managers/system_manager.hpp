#pragma once
#include <engine/world/system.hpp>
#include <flecs.h>
#include <unordered_map>
#include <string>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class csystem_manager final
	{
	public:
		using systems_t = std::unordered_map<std::string, flecs::system>;

		csystem_manager() = default;
		~csystem_manager() = default;

		static auto		builtin_phase(std::string_view name) -> std::pair<bool, uint64_t>;

		void			world(flecs::world* w) { m_world = w; };
		void			init();
		void			shutdown();

		template<typename... TComps>
		void			create_system(const kokoro::system::sconfig& cfg, kokoro::system::system_callback_t<TComps...> callback);
		void			create_task(const kokoro::system::sconfig& cfg, kokoro::system::task_callback_t callback);
		flecs::system	find_system(std::string_view name);

	private:
		flecs::world* m_world = nullptr;
		systems_t m_all_systems;				//- All created systems for world
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps>
	void kokoro::csystem_manager::create_system(const kokoro::system::sconfig& cfg, kokoro::system::system_callback_t<TComps...> callback)
	{
		auto builder = m_world->system<TComps...>(cfg.m_name.c_str());

		//- Set options that are required before system entity creation
		{
			if (!!(cfg.m_flags & system::system_flag_multithreaded))
			{
				builder.multi_threaded();
			}
			else if (!!(cfg.m_flags & system::system_flag_immediate))
			{
				builder.immediate();
			}
		}

		if (const auto duplicate_system = find_system(cfg.m_name); duplicate_system)
		{
			return;
		}

		if (cfg.m_interval != 0)
		{
			builder.interval(static_cast<float>(cfg.m_interval));
		}

		auto system = builder.each(callback);

		//- Set options that are required after system entity creation
		{
			for (const auto& after : cfg.m_run_after)
			{
				if (const auto [result, phase] = builtin_phase(after); result)
				{
					system.add(flecs::Phase).depends_on(phase);
				}
				else
				{
					if (auto e = find_system(after.c_str()); e)
					{
						system.add(flecs::Phase).depends_on(e);
					}
				}
			}

			for (const auto& before : cfg.m_run_before)
			{
				if (const auto [result, phase] = builtin_phase(before); result)
				{
					system.add(flecs::Phase).depends_on(phase);
				}
				else
				{
					if (auto e = find_system(before.c_str()); e)
					{
						e.add(flecs::Phase).depends_on(system);
					}
				}
			}
		}
		m_all_systems[cfg.m_name] = system;
	}

} //- kokoro