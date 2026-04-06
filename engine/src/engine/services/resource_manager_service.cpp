#include <engine/services/resource_manager_service.hpp>

namespace kokoro
{
	namespace
	{
		constexpr auto C_GARBAGE_COLLECT_INTERVAL = 5.0f;
		float accumulator = 0.0f;

	} //- unnamed

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
	void cresource_manager_service::update(float dt)
	{
		//- Iterate caches and let them commit the results of loading
		for (auto& [_, desc] : m_cache_descs)
		{
			desc.m_cache->commit();
		}

		accumulator += dt;

		//- Do periodic garbage collection for resources that are no longer in use
		while (accumulator >= C_GARBAGE_COLLECT_INTERVAL)
		{
			accumulator -= C_GARBAGE_COLLECT_INTERVAL;

			for (auto& [_, desc] : m_cache_descs)
			{
				desc.m_cache->update(dt);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	cresource_manager_service::sresource_cache_desc::sresource_cache_desc(std::unique_ptr<icache>&& cache, rttr::type resource_type, rttr::type snapshot_type, uint64_t next_id) :
		m_cache(std::move(cache)),
		m_resource_type(resource_type),
		m_snapshot_type(snapshot_type),
		m_next_id(next_id)
	{
	}

	//------------------------------------------------------------------------------------------------------------------------
	cresource_manager_service::sresource_cache_desc::sresource_cache_desc(sresource_cache_desc&& other) noexcept :
		m_cache(std::move(other.m_cache)),
		m_resource_type(other.m_resource_type),
		m_snapshot_type(other.m_snapshot_type),
		m_next_id(other.m_next_id)
	{
	}

} //- kokoro