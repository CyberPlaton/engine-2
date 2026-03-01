#include <engine/services/command_service.hpp>

namespace kokoro
{
	namespace command
	{
		//------------------------------------------------------------------------------------------------------------------------
		icommand::icommand(world::sworld* w, std::string_view pawn_name_or_uuid)
			: m_pawn(invalid_pawn_id_t)
			, m_world(w)
		{
			if (!pawn_name_or_uuid.empty())
			{
				if (auto e = world::entity::find(*m_world, pawn_name_or_uuid); e.is_valid())
				{
					m_pawn = e.id();
				}
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		flecs::entity icommand::pawn() const
		{
			return world().m_world.entity(pawn_id());
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool icommand::has_pawn() const
		{
			return pawn().is_valid();
		}

		//------------------------------------------------------------------------------------------------------------------------
		auto icommand::world() const -> const world::sworld&
		{
			return *m_world;
		}

		//------------------------------------------------------------------------------------------------------------------------
		world::sworld& icommand::world()
		{
			return *m_world;
		}

	} //- command

	//------------------------------------------------------------------------------------------------------------------------
	ccommand_system_service::~ccommand_system_service()
	{
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool ccommand_system_service::init()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ccommand_system_service::post_init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void ccommand_system_service::shutdown()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void ccommand_system_service::update(float dt)
	{
		constexpr auto C_COMMAND_BATCH_COUNT = 64;

		std::vector<std::unique_ptr<command::icommand>> queue;

		if (m_queue.size() > C_COMMAND_BATCH_COUNT)
		{
			//- Take the first commands to be batch executed and leave the rest for next frames
			queue.resize(C_COMMAND_BATCH_COUNT);
			core::cscoped_mutex m(m_mutex);

			for (auto i = 0; i < C_COMMAND_BATCH_COUNT; ++i)
			{
				queue[i] = std::move(m_queue[i]);
			}
			m_queue.erase(m_queue.begin(), m_queue.begin() + C_COMMAND_BATCH_COUNT);
		}
		else
		{
			queue.resize(m_queue.size());
			core::cscoped_mutex m(m_mutex);

			//- Take the whole command queue and execute all
			for (auto i = 0; i < m_queue.size(); ++i)
			{
				queue[i] = std::move(m_queue[i]);
			}
			m_queue.clear();
		}

		//- Execute submitted commands in a sequential manner
		for (auto& q : queue)
		{
			if (q->prepare())
			{
				q->execute();
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ccommand_system_service::push(std::unique_ptr<command::icommand>&& command)
	{
		core::cscoped_mutex m(m_mutex);
		m_queue.push_back(std::move(command));
	}

} //- kokoro