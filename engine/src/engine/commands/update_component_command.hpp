#pragma once
#include <engine/services/command_service.hpp>

namespace kokoro
{
	namespace command
	{
		//------------------------------------------------------------------------------------------------------------------------
		class cupdate_component_command final : public command::icommand
		{
		public:
			cupdate_component_command(world::sworld* w, std::string_view pawn_name_or_uuid, rttr::variant&& component);

			bool prepare() override final;
			void execute() override final;
			bool rollback() override final;

		private:
			rttr::variant m_previous_component;
			rttr::variant m_new_component;

		private:
			bool set(const rttr::variant& component);
		};

	} //- command

} //- kokoro