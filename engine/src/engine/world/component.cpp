#include <engine/world/component.hpp>
#include <registrator.hpp>

RTTR_REGISTRATION
{
	using namespace kokoro::ecs;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<icomponent>("icomponent");
}