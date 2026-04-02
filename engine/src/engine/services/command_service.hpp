#pragma once
#include <engine/iservice.hpp>
#include <engine/services/resource_manager_service.hpp>
#include <engine/world/world.hpp>
#include <core/mutex.hpp>

namespace kokoro
{
	using pawn_id_t = uint64_t;
	#define invalid_pawn_id_t std::numeric_limits<pawn_id_t>().max()

	namespace command
	{
		//- Interface for a generic command that can be used to request a state change for an entity or object in the game world
		//------------------------------------------------------------------------------------------------------------------------
		class icommand
		{
		public:
			icommand(cview<cworld> w, std::string_view pawn_name_or_uuid);

			//- Prepare command for execution, on failure it will not be executed.
			virtual bool	prepare() { return false; };

			//- Perform the command logic. Inside this function real state change happens.
			virtual void	execute() {};

			//- Revert the command, indicates if succeeded or failed.
			virtual bool	rollback() { return false; };

			pawn_id_t		pawn_id() const { return m_pawn; }
			flecs::entity	pawn() const;
			bool			has_pawn() const;
			auto			world() -> cview<cworld> { return m_world; }

		private:
			pawn_id_t m_pawn;
			cview<cworld> m_world;
		};

	} //- command

	//- Responsible for executing submitted commands sequentially in a secure manner without causing race conditions.
	//------------------------------------------------------------------------------------------------------------------------
	class ccommand_system_service final : public iservice
	{
	public:
		ccommand_system_service() = default;
		~ccommand_system_service();

		bool		init() override final;
		void		post_init() override final;
		void		shutdown() override final;
		void		update(float dt) override final;

		template<typename TCommand, typename... ARGS>
		void		push(ARGS&&... args);
		void		push(std::unique_ptr<command::icommand>&& command);

	private:
		core::cmutex m_mutex;
		std::vector<std::unique_ptr<command::icommand>> m_queue;
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TCommand, typename... ARGS>
	void ccommand_system_service::push(ARGS&&... args)
	{
		push(std::move(std::make_unique<TCommand>(std::forward<ARGS>(args)...)));
	}

} //- kokoro