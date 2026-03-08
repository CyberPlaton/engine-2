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
		for (const auto& l : m_layers)
		{
			if (!l->init())
			{
				return false;
			}
		}

		for (const auto& s : m_services)
		{
			if (!s->init())
			{
				return false;
			}
		}

		for (const auto& l : m_layers)
		{
			l->post_init();
		}

		for (const auto& s : m_services)
		{
			s->post_init();
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cengine::shutdown()
	{
		for (const auto& l : m_layers)
		{
			l->shutdown();
		}

		for (const auto& s : m_services)
		{
			s->shutdown();
		}

		m_layers.clear();
		m_services.clear();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cengine::run()
	{
		m_running = true;

		auto& rs = instance().service<crender_service>();

		while (m_running)
		{
			const auto dt = 0.033f;

			rs.begin_frame();

			for (const auto& s : m_services)
			{
				s->update(dt);
			}

			for (const auto& l : m_layers)
			{
				l->update(dt);
			}

			rs.end_frame();
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cengine::exit()
	{
		m_running = false;
	}

} //- kokoro