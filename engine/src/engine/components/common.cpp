#include <engine/components/common.hpp>

namespace kokoro::world::component
{
	//------------------------------------------------------------------------------------------------------------------------
	bool sidentifier::show_ui(rttr::variant& comp)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool shierarchy::show_ui(rttr::variant& comp)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool sprefab::show_ui(rttr::variant& comp)
	{
		return false;
	}

} //- kokoro::world::component

RTTR_REGISTRATION
{
	using namespace kokoro::world;

	rttr::detail::default_constructor<std::vector<kokoro::core::cuuid>>();

	REGISTER_TAG(sinvisible);
	REGISTER_TAG(sentity);

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<component::sidentifier>("sidentifier")
		.prop("m_name", &component::sidentifier::m_name)
		.prop("m_uuid", &component::sidentifier::m_uuid)
		;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<component::shierarchy>("shierarchy")
		.prop("m_parent", &component::shierarchy::m_parent)
		.prop("m_children", &component::shierarchy::m_children)
		;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<component::sprefab>("sprefab")
		.prop("m_filepath", &component::sprefab::m_filepath)
		;
}