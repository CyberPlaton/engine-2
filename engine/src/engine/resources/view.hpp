#pragma once
#include <engine/resources/cache.hpp>

namespace kokoro
{
	//- A view of a resource allows access to data stored and additional information about the resource. A valid resource
	//- has a real id and is stored in the resource cache, however, it might not be completely loaded yet. A ready resource
	//- has completed loading either successfully or not.
	//- Depending on the resource we are viewing the path may be empty. This implies that it was loaded from a direct snapshot
	//- object and is unmanaged (non-owning) and must be explicitly unloaded once.
	//- When an unmanaged resource is unloaded, other views viewing said resource become invalid.
	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	class cview final
	{
	public:
		explicit cview(resource_id_t id, ccache<TType>* cache);
		cview() = default;
		cview(const cview& other);
		cview& operator=(const cview& other);
		cview(cview&& other) noexcept;
		cview& operator=(cview&& other) noexcept;
		~cview();

		inline TType&			get() { return m_cache->get(m_id); }
		inline const TType&		get() const { return const_cast<const TType&>(m_cache->get(m_id)); }
		filepath_t				path() const; //- Retrieve the path from which resource was loaded. May be empty if loaded from snapshot directly.
		resource_state			state() const;
		resource_ownership		ownership() const;
		inline bool				valid() const { return m_cache && m_id != invalid_id_t && m_cache->valid(m_id); }
		inline resource_id_t	id() const { return m_id; }
		inline bool				ready() const { return state() == resource_state_finished; }
		uint32_t				use_count() const;
		operator				bool() const { return valid() && ready(); }

	private:
		ccache<TType>* m_cache = nullptr;
		resource_id_t m_id = invalid_id_t;
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	uint32_t cview<TType>::use_count() const
	{
		if (!m_cache || m_id == invalid_id_t)
		{
			return 0;
		}
		return m_cache->use_count(m_id);
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	resource_ownership cview<TType>::ownership() const
	{
		if (!m_cache || m_id == invalid_id_t)
		{
			return resource_ownership_none;
		}
		return m_cache->ownership(m_id);
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	cview<TType>::cview(const cview& other) :
		m_cache(other.m_cache),
		m_id(other.m_id)
	{
		if (m_cache && m_id != invalid_id_t)
		{
			m_cache->increase_ref_count(m_id);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	cview<TType>& cview<TType>::operator=(const cview& other)
	{
		if (this != &other)
		{
			if (m_cache && m_id != invalid_id_t)
			{
				m_cache->decrease_ref_count(m_id);
			}

			m_cache = other.m_cache;
			m_id = other.m_id;

			if (m_cache && m_id != invalid_id_t)
			{
				m_cache->increase_ref_count(m_id);
			}
		}
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	cview<TType>::cview(cview&& other) noexcept :
		m_cache(other.m_cache),
		m_id(other.m_id)
	{
		other.m_cache = nullptr;
		other.m_id = invalid_id_t;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	cview<TType>& cview<TType>::operator=(cview&& other) noexcept
	{
		if (this != &other)
		{
			if (m_cache && m_id != invalid_id_t)
			{
				m_cache->decrease_ref_count(m_id);
			}

			m_cache = other.m_cache;
			m_id = other.m_id;

			other.m_cache = nullptr;
			other.m_id = invalid_id_t;
		}
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	resource_state cview<TType>::state() const
	{
		if (!m_cache || m_id == invalid_id_t)
		{
			return resource_state_none;
		}
		return m_cache->state(m_id);
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	filepath_t cview<TType>::path() const
	{
		if (!m_cache || m_id == invalid_id_t)
		{
			return {};
		}
		return m_cache->path(m_id);
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	cview<TType>::cview(resource_id_t id, ccache<TType>* cache) :
		m_cache(cache),
		m_id(id)
	{
		if (m_cache && id != invalid_id_t)
		{
			m_cache->increase_ref_count(m_id);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	cview<TType>::~cview()
	{
		if (m_cache && m_id != invalid_id_t)
		{
			m_cache->decrease_ref_count(m_id);
		}
	}

} //- kokoro