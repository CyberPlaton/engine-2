#pragma once
#include <cstdint>
#include <engine/services/virtual_filesystem_service.hpp>
#include <core/mutex.hpp>
#include <core/profile.hpp>
#include <engine.hpp>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <tuple>

namespace kokoro
{
	class cresource_manager_service;
	using resource_type_id_t = uint64_t;
	using resource_id_t = uint64_t;
	using hashed_path_t = uint64_t;
#define invalid_id_t std::numeric_limits<resource_id_t>::max()

	//------------------------------------------------------------------------------------------------------------------------
	enum resource_state : uint8_t
	{
		resource_state_none = 0,
		resource_state_pending,
		resource_state_finished,
		resource_state_failed,
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	struct sresource final
	{
	public:
		TType m_data;
		filepath_t m_path;
		resource_id_t m_id = invalid_id_t;
		resource_state m_state = resource_state_none;
	};

	//------------------------------------------------------------------------------------------------------------------------
	class icache
	{
	public:
		virtual ~icache() = default;

		virtual void commit() = 0;
		virtual void shutdown() = 0;
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	class ccache final : public icache
	{
		friend class cresource_manager_service;
	public:
		using resource_t = sresource<TType>;

		ccache() = default;
		~ccache() = default;

		void			commit() override final;
		void			shutdown() override final;
		TType&			get(resource_id_t id);
		filepath_t		path(resource_id_t id) const;
		resource_state	state(resource_id_t id) const;
		bool			valid(resource_id_t id) const;

	private:
		std::unordered_map<resource_id_t, resource_t> m_entries;
		mutable core::cmutex m_mutex;
		std::queue<std::tuple<bool, resource_id_t, TType>> m_pending_load;
		std::unordered_set<resource_id_t> m_pending_unload;
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	void ccache<TType>::shutdown()
	{
		core::cscoped_mutex m(m_mutex);

		m_pending_unload.clear();

		//- Before we can unload everything, we have to wait for any pending loading tasks
		while (!m_pending_load.empty())
		{
			auto& [success, id, data] = m_pending_load.back();
			auto& entry = m_entries[id];
			if (success)
			{
				entry.m_data = std::move(data);
			}
			m_pending_load.pop();
		}

		//- Unload all valid and loaded resources
		for (auto& [_, resource] : m_entries)
		{
			TType::unload(resource.m_data);
		}
		m_entries.clear();
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	bool ccache<TType>::valid(resource_id_t id) const
	{
		core::cscoped_mutex m(m_mutex);
		return m_entries.find(id) != m_entries.end();
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	resource_state ccache<TType>::state(resource_id_t id) const
	{
		core::cscoped_mutex m(m_mutex);
		if (const auto it = m_entries.find(id); it != m_entries.end())
		{
			return it->second.m_state;
		}
		return resource_state_none;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	filepath_t ccache<TType>::path(resource_id_t id) const
	{
		core::cscoped_mutex m(m_mutex);
		if (const auto it = m_entries.find(id); it != m_entries.end())
		{
			return it->second.m_path;
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	TType& ccache<TType>::get(resource_id_t id)
	{
		core::cscoped_mutex m(m_mutex);
		if (const auto it = m_entries.find(id); it != m_entries.end())
		{
			return it->second.m_data;
		}
		static TType S_DUMMY;
		return S_DUMMY;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	void ccache<TType>::commit()
	{
		CPU_ZONE;

		auto& log = instance().service<clog_service>();
		const auto resource_type = rttr::type::get<TType>();

		//- Function moves resources from pending to use-ready entries
		//- and updates its state and assigns a valid id
		core::cscoped_mutex m(m_mutex);
		while (!m_pending_load.empty())
		{
			auto& [success, id, data] = m_pending_load.front();
			auto& entry = m_entries.find(id)->second;

			const auto text = fmt::format("{} resource '{} (id={}, type={})'",
				success ? "Successfully loaded" : "Failed loading",
				m_entries.find(id)->second.m_path.generic_string(),
				id,
				resource_type.get_name().data());

			if (success)
			{
				//- Note, id and path are assigned by the resource manager
				entry.m_data = std::move(data);
				entry.m_state = resource_state_finished;

				log.info(text.c_str());
			}
			else
			{
				entry.m_state = resource_state_failed;

				log.err(text.c_str());
			}

			m_pending_load.pop();

			//- Check if an unload was scheduled for this resource, and if so, perform it here
			if (m_pending_unload.count(id) > 0)
			{
				TType::unload(entry.m_data);
				m_entries.erase(id);
				m_pending_unload.erase(id);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	class cview final
	{
	public:
		explicit cview(resource_id_t id, ccache<TType>* cache);
		cview() = default;
		~cview() = default;

		inline TType&			get() { return m_cache->get(m_id); }
		inline const TType&		get() const { return const_cast<const TType&>(m_cache->get(m_id)); }
		filepath_t				path() const;
		resource_state			state() const;
		inline bool				valid() const { return m_cache && m_id != invalid_id_t && m_cache->valid(m_id); }
		inline resource_id_t	id() const { return m_id; }
		inline bool				ready() const { return state() == resource_state_finished; }
		operator				bool() const { return valid() && ready(); }

	private:
		ccache<TType>* m_cache = nullptr;
		resource_id_t m_id = invalid_id_t;
	};

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
	cview<TType>::cview(resource_id_t id, ccache<TType>* cache) : m_cache(cache), m_id(id) {}

} //- kokoro