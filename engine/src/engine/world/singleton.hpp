#pragma once
#include <engine/world/component.hpp>

//- Macro for defining a singleton component. Use this explicitly or for component
#define DECLARE_SINGLETON(c) \
DECLARE_COMPONENT_NO_UI(c)

namespace kokoro
{
	namespace ecs
	{
		//- Base class for all singletons
		//------------------------------------------------------------------------------------------------------------------------
		struct isingleton : icomponent
		{
			static std::string_view name() { static constexpr std::string_view C_NAME = "isingleton"; return C_NAME; };

			RTTR_ENABLE(icomponent)
		};

	} //- ecs

} //- kokoro