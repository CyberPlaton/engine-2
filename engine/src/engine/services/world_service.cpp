#include <engine/services/world_service.hpp>
#include <engine.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	bool cworld_service::init()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cworld_service::post_init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cworld_service::shutdown()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cworld_service::update(float dt)
	{
		if (!m_active)
		{
			//- Either no world set to be active or the world is not ready yet
			return;
		}
		m_active.get().tick(dt);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cworld_service::promote(cview<cworld> world)
	{
		m_active = world;
	}

} //- kokoro