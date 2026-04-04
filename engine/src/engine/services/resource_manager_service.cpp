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

	//------------------------------------------------------------------------------------------------------------------------
	cresource_manager_service::sresource_cache_desc::sresource_cache_desc(std::unique_ptr<icache>&& cache, rttr::type resource_type, rttr::type snapshot_type, uint64_t next_id, bool unique_instances) :
		m_cache(std::move(cache)),
		m_resource_type(resource_type),
		m_snapshot_type(snapshot_type),
		m_next_id(next_id),
		m_unique_instances(unique_instances)
	{
	}

	//------------------------------------------------------------------------------------------------------------------------
	cresource_manager_service::sresource_cache_desc::sresource_cache_desc(sresource_cache_desc&& other) noexcept :
		m_cache(std::move(other.m_cache)),
		m_resource_type(other.m_resource_type),
		m_snapshot_type(other.m_snapshot_type),
		m_next_id(other.m_next_id),
		m_unique_instances(other.m_unique_instances)
	{
	}

} //- kokoro