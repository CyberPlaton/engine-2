#include <engine/world/managers/system_manager.hpp>
#include <engine/components/common.hpp>
#include <engine/services/log_service.hpp>
#include <engine.hpp>
#include <fmt.h>

namespace kokoro
{
	//- Function responsible for creating a system for a world with given configuration, without matching any components
	//- and function to execute.
	//- A task is similar to a normal system, only that it does not match any components and thus no entities.
	//- If entities are required they can be retrieved through the world or a query.
	//- The function itself is executed as is, with only delta time provided.
	//------------------------------------------------------------------------------------------------------------------------
	void csystem_manager::create_task(const kokoro::system::sconfig& cfg, kokoro::system::task_callback_t callback)
	{
		auto builder = m_world->system(cfg.m_name.c_str());

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
			instance().service<clog_service>().err("Trying to create a system with same name twice '{}'. This is not allowed!",
				cfg.m_name);
			return;
		}

		if (cfg.m_interval != 0)
		{
			builder.interval(static_cast<float>(cfg.m_interval));
		}

		//- Create function to be called for running the task
		auto function = [=](flecs::iter& it)
			{
				(callback)(it, it.delta_time());
			};

		auto task = builder.run(std::move(function));

		//- Set options that are required after system entity creation
		{
			for (const auto& after : cfg.m_run_after)
			{
				if (const auto [result, phase] = builtin_phase(after); result)
				{
					task.add(flecs::Phase).depends_on(phase);
				}
				else
				{
					if (auto e = find_system(after); e)
					{
						task.add(flecs::Phase).depends_on(e);
					}
					else
					{
						instance().service<clog_service>().err("Dependency (run after) system '{}' for system '{}' could not be found!",
							after, cfg.m_name);
					}
				}
			}

			for (const auto& before : cfg.m_run_before)
			{
				if (const auto [result, phase] = builtin_phase(before); result)
				{
					task.add(flecs::Phase).depends_on(phase);
				}
				else
				{
					if (auto e = find_system(before); e)
					{
						e.add(flecs::Phase).depends_on(task);
					}
					else
					{
						instance().service<clog_service>().err("Dependent (run before) system '{}' for system '{}' could not be found!",
							before, cfg.m_name);
					}
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	flecs::system csystem_manager::find_system(std::string_view name)
	{
		if (const auto it = m_all_systems.find(name.data()); it != m_all_systems.end())
		{
			return it->second;
		}
		return flecs::system{};
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto csystem_manager::builtin_phase(std::string_view name) -> std::pair<bool, uint64_t>
	{
		static std::array<std::pair<std::string, uint64_t>, 8> C_PHASES =
		{
			std::pair("OnLoad",		(uint64_t)flecs::OnLoad),
			std::pair("PostLoad",	(uint64_t)flecs::PostLoad),
			std::pair("PreUpdate",	(uint64_t)flecs::PreUpdate),
			std::pair("OnUpdate",	(uint64_t)flecs::OnUpdate),
			std::pair("OnValidate", (uint64_t)flecs::OnValidate),
			std::pair("PostUpdate", (uint64_t)flecs::PostUpdate),
			std::pair("PreStore",	(uint64_t)flecs::PreStore),
			std::pair("OnStore",	(uint64_t)flecs::OnStore)
		};

		if (const auto& it = std::find_if(C_PHASES.begin(), C_PHASES.end(), [=](const auto& pair)
			{
				return pair.first == name;

			}); it != C_PHASES.end())
		{
			return { true, (uint64_t)it->second };
		}

		return { false, std::numeric_limits<uint64_t>().max() };
	}

	//------------------------------------------------------------------------------------------------------------------------
	void csystem_manager::init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void csystem_manager::shutdown()
	{

	}

} //- kokoro