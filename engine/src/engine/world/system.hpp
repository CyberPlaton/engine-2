#pragma once
#include <core/config.hpp>
#include <rttr.h>
#include <flecs.h>

//- Use macro to reflect your system, the system function must be declared and implemented.
//-------------------------------------------------------------------------------------------------------------------------
#define REGISTER_SYSTEM(s) \
	rttr::cregistrator<s>(STRINGIFY(s)) \
		.ctor<kokoro::world::sworld&>() \
		.meth(kokoro::system::ssystem::C_SYSTEM_CONFIG_FUNC_NAME, &s::config) \
		;

//-------------------------------------------------------------------------------------------------------------------------
#define DECLARE_SYSTEM(s, f) \
s() = default; \
s(kokoro::world::sworld& w) { kokoro::world::modules::create_system(w, config(), f); } \
static kokoro::system::sconfig config(); \
RTTR_ENABLE(); public:

//-------------------------------------------------------------------------------------------------------------------------
#define DECLARE_TASK(t, f) \
t() = default; \
t(kokoro::world::sworld& w) { kokoro::world::modules::create_task(w, config(), f); } \
static kokoro::system::sconfig config(); \
RTTR_ENABLE(); public:

namespace kokoro
{
	namespace system
	{
		template<typename... TComps>
		using system_callback_t = void(flecs::entity, TComps...);
		using task_callback_t = void(flecs::iter&, float);
		template<typename... TComps>
		using task_callback_var_t = void(flecs::iter&, size_t, TComps...);
		using system_flags_t = int;

		//------------------------------------------------------------------------------------------------------------------------
		enum system_flag : uint8_t
		{
			system_flag_none = 0,
			system_flag_multithreaded,	//- Indicates that a system can be run multithread
			system_flag_immediate,		//- Disable read-only mode for system on tick, meaning a change will directly be visible.
										//- Note: immediate is exclusive with multithreaded.
		};

		//------------------------------------------------------------------------------------------------------------------------
		struct sconfig final
		{
			std::string m_name;
			vector_t<std::string> m_run_after;
			vector_t<std::string> m_run_before;
			uint32_t m_interval		= 0;
			system_flags_t m_flags	= 0;
		};

		//- This is a dummy system to show how one should be defined. While creating you must not inherit from it.
		//- Define all required functions and reflect them to RTTR using the macro REGISTER_SYSTEM().
		//------------------------------------------------------------------------------------------------------------------------
		struct ssystem final
		{
			static constexpr rttr::string_view C_SYSTEM_CONFIG_FUNC_NAME = "config";
			static constexpr array_t<rttr::string_view, 1> C_SYSTEM_FUNC_NAMES = { C_SYSTEM_CONFIG_FUNC_NAME };

			static sconfig config() { return {}; }

			RTTR_ENABLE()
		};

	} //- system

} //- kokoro