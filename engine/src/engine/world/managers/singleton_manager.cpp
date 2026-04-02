#include <engine/world/managers/singleton_manager.hpp>
#include <engine/world/component.hpp>

namespace kokoro
{
	namespace
	{
		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		rttr::variant invoke_static_function(rttr::type class_type, rttr::string_view function_name, ARGS&&... args)
		{
			if (const auto m = class_type.get_method(function_name); m.is_valid())
			{
				return m.invoke({}, args...);
			}
			return {};
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	void csingleton_manager::init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void csingleton_manager::shutdown()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	auto csingleton_manager::registered() -> component_types_t
	{
		return m_all_singletons;
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto csingleton_manager::all() -> component_types_t
	{
		return m_active_singletons;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void csingleton_manager::add(const rttr::variant& c)
	{
		invoke_static_function(c.get_type(), kokoro::ecs::C_COMPONENT_ADD_SINGLETON_FUNC_NAME, m_world);
		m_active_singletons.insert(c.get_type().get_name().data());
	}

	//------------------------------------------------------------------------------------------------------------------------
	void csingleton_manager::remove(const rttr::type& t)
	{
		invoke_static_function(t, kokoro::ecs::C_COMPONENT_REMOVE_SINGLETON_FUNC_NAME, m_world);
		m_active_singletons.erase(t.get_name().data());
	}

} //- kokoro