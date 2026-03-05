#include <engine/components/viewport.hpp>
#include <registrator.hpp>

namespace kokoro::world::component
{

} //- kokoro::world::component

RTTR_REGISTRATION
{
	using namespace kokoro::world::component;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<sviewport>("sviewport");
}