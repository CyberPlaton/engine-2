#include <engine.hpp>
#include <engine/services/render_service.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	cengine& cengine::instance()
	{
		static cengine S_INSTANCE;
		return S_INSTANCE;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cengine& instance()
	{
		return cengine::instance();
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cengine::init()
	{
		for (const auto& [_, s] : m_services)
		{
			if (!s->init())
			{
				return false;
			}
		}

		for (const auto& [_, s] : m_services)
		{
			s->post_init();
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cengine::shutdown()
	{
		for (const auto& [_, s] : m_services)
		{
			s->shutdown();
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cengine::run()
	{
		m_running = true;

		auto& rs = instance().service<crender_service>();

		while (m_running)
		{
			const auto dt = 0.033f;

			for (const auto& [_, s] : m_services)
			{
				s->update(dt);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cengine::exit()
	{
		m_running = false;
	}

} //- kokoro