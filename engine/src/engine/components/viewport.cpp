#include <engine/components/viewport.hpp>

namespace kokoro::world::component
{
	//------------------------------------------------------------------------------------------------------------------------
	bool sviewport::show_ui(rttr::variant& comp)
	{
		return false;
	}

} //- kokoro::world::component

RTTR_REGISTRATION
{
	using namespace kokoro::world::component;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<sviewport>("sviewport");
}