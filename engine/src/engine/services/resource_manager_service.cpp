#include <engine/services/resource_manager_service.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	const cresource_manager_service::sresource_cache_desc& cresource_manager_service::cache_desc(resource_type_id_t type_id) const
	{
		return m_cache_descs.at(type_id);
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cresource_manager_service::init()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cresource_manager_service::shutdown()
	{
		for (auto& [_, desc] : m_cache_descs)
		{
			desc.m_cache->shutdown();
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cresource_manager_service::update(float)
	{
		//- Iterate caches and let them commit the results of loading
		for (auto& [_, desc] : m_cache_descs)
		{
			desc.m_cache->commit();
		}
	}

} //- kokoro