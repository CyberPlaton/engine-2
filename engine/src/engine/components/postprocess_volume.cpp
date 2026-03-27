#include <engine/components/postprocess_volume.hpp>

namespace kokoro::world::component
{
	//------------------------------------------------------------------------------------------------------------------------
	bool spostprocess_volume::show_ui(rttr::variant& comp)
	{
		return false;
	}

} //- kokoro::world::component

RTTR_REGISTRATION
{
	using namespace kokoro::world::component;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<spostprocess_volume>("spostprocess_volume");
}