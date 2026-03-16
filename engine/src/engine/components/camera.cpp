#include <engine/components/camera.hpp>

namespace kokoro::world::component
{
	//------------------------------------------------------------------------------------------------------------------------
	bool scamera::show_ui(rttr::variant& comp)
	{
		return false;
	}

} //- kokoro::world::component

RTTR_REGISTRATION
{
	using namespace kokoro::world::component;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<scamera>("scamera");
}