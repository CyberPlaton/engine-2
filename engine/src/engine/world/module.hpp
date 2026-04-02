#pragma once
#include <rttr.h>
#include <flecs.h>
#include <vector>

//- Use macro to reflect your module, the module functions must be declared and implemented.
//-------------------------------------------------------------------------------------------------------------------------
#define REGISTER_MODULE(m) \
	rttr::cregistrator<m>(#m) \
		.ctor<kokoro::cworld*>() \
		.meth("config", &m::config) \
		;

//-------------------------------------------------------------------------------------------------------------------------
#define DECLARE_MODULE(m) \
m() = default; \
m(const m&) = default; \
m(kokoro::cworld* w) { w->import_module<m>(config()); } \
static kokoro::modules::sconfig config(); \
RTTR_ENABLE()

namespace kokoro
{
	namespace modules
	{
		//-------------------------------------------------------------------------------------------------------------------------
		struct sconfig final
		{
			std::string m_name;
			std::vector<std::string> m_components;	//- Components that this module is using
			std::vector<std::string> m_systems;		//- Systems that this module is using
			std::vector<std::string> m_modules;		//- Dependent modules that must be loaded before this one
		};

		//- This is a dummy module to show how one should be defined. While creating one you must not inherit from it.
		//- Define all required functions and reflect them to RTTR using the macro REGISTER_MODULE().
		//- Note: use DECLARE_MODULE(...) inside module struct and implement the config function, thats it.
		//------------------------------------------------------------------------------------------------------------------------
		struct smodule final
		{
			static sconfig config() { return {}; }

			RTTR_ENABLE()
		};

	} //- modules

} //- kokoro