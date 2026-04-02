#include <engine/commands/update_component_command.hpp>
#include <engine/world/component.hpp>
#include <engine/world/world.hpp>
#include <engine/services/log_service.hpp>
#include <engine.hpp>
#include <core/registrator.hpp>
#include <fmt.h>

namespace kokoro
{
	namespace command
	{
		//------------------------------------------------------------------------------------------------------------------------
		cupdate_component_command::cupdate_component_command(cview<cworld> w, std::string_view pawn_name_or_uuid, rttr::variant&& component) :
			command::icommand(w, pawn_name_or_uuid), m_new_component(std::move(component))
		{
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool cupdate_component_command::prepare()
		{
			//- We have to ensure the new component is a valid type
			const auto type = m_new_component.get_type();

			if (has_pawn() && type.is_valid())
			{
				const auto has_method = type.get_method(ecs::C_COMPONENT_HAS_FUNC_NAME.data());
				const auto get_method = type.get_method(ecs::C_COMPONENT_GET_COMPONENT_FUNC_NAME.data());

				//- If entity has the component we can store it as previous component
				if (has_method.is_valid() && get_method.is_valid())
				{
					if (has_method.invoke({}, pawn()).to_bool())
					{
						m_previous_component = get_method.invoke({}, pawn());
					}
				}
				return true;
			}
			return false;
		}

		//------------------------------------------------------------------------------------------------------------------------
		void cupdate_component_command::execute()
		{
			set(m_new_component);
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool cupdate_component_command::rollback()
		{
			return set(m_previous_component);
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool cupdate_component_command::set(const rttr::variant& component)
		{
			if (!component.is_valid())
			{
				instance().service<clog_service>().trace("Component update command trying to set an invalid component");
				return false;
			}

			if (const auto type = component.get_type(); type.is_valid())
			{
				if (const auto set_method = type.get_method(ecs::C_COMPONENT_SET_FUNC_NAME.data());  set_method.is_valid())
				{
					set_method.invoke({}, pawn(), component);
					return true;
				}
			}
			return false;
		}

	} //- command

} //- kokoro
