#include <engine/services/command_service.hpp>

namespace kokoro
{
	namespace command
	{
		//------------------------------------------------------------------------------------------------------------------------
		icommand::icommand(cview<cworld> w, std::string_view pawn_name_or_uuid)
			: m_pawn(invalid_pawn_id_t)
			, m_world(w)
		{
			if (!pawn_name_or_uuid.empty() && m_world.valid())
			{
				if (auto e = m_world.get().entity_manager().find(pawn_name_or_uuid); e.is_valid())
				{
					m_pawn = e.id();
				}
			}
		}

		//------------------------------------------------------------------------------------------------------------------------
		flecs::entity icommand::pawn() const
		{
			if (m_world.valid())
			{
				m_world.get().w().entity(pawn_id());
			}
			return {};
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool icommand::has_pawn() const
		{
			return pawn().is_valid();
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