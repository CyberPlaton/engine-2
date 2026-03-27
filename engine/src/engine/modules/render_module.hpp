#pragma once
#include <engine/world/module.hpp>
#include <engine/world/world.hpp>

namespace kokoro
{
	//- Engine module containing default render systems, components etc.
	//------------------------------------------------------------------------------------------------------------------------
	struct srender_module final
	{
		DECLARE_MODULE(srender_module);
	};

} //- kokoro