#pragma once
#include <core/mutex.hpp>
#include <core/profile.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <fmt.h>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <queue>

namespace kokoro
{
	class cresource_manager_service;
	template<typename TType> class cview;
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
	enum resource_ownership : uint8_t
	{
		resource_ownership_none = 0,
		resource_ownership_owning,
		resource_ownership_non_owning,
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	struct sresource final
	{
	public:
		std::optional<TType> m_data		= std::nullopt;
		filepath_t m_path				= {};
		resource_id_t m_id				= invalid_id_t;
		resource_state m_state			= resource_state_none;
		resource_ownership m_ownership	= resource_ownership_none;
		uint32_t m_ref_count			= 0;
	};

	//------------------------------------------------------------------------------------------------------------------------
	class icache
	{
	public:
		virtual ~icache() = default;
		virtual void commit() = 0;
		virtual void shutdown() = 0;
		virtual void update(float) = 0;
	};


	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	class ccache final : public icache
	{
		friend class cview<TType>;
		friend class cresource_manager_service;
	public:
		using resource_t = sresource<TType>;

		ccache() = default;
		~ccache() = default;

		void			commit() override final;
		void			shutdown() override final;
		void			update(float dt) override final;
		TType&			get(resource_id_t id);
		filepath_t		path(resource_id_t id) const;
		resource_state	state(resource_id_t id) const;
		bool			valid(resource_id_t id) const;

	private:
		std::unordered_map<resource_id_t, resource_t> m_entries;
		mutable core::cmutex m_mutex;
		std::queue<std::pair<resource_id_t, std::optional<TType>>> m_pending_load;
		std::unordered_set<resource_id_t> m_pending_unload;

	private:
		void			increase_ref_count(resource_id_t id);
		void			decrease_ref_count(resource_id_t id);
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	void ccache<TType>::decrease_ref_count(resource_id_t id)
	{
		core::cscoped_mutex m(m_mutex);

		if (auto it = m_entries.find(id); it != m_entries.end())
		{
			if (it->second.m_ref_count > 0 &&
				it->second.m_state != resource_state_pending)
			{
				--it->second.m_ref_count;
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	void ccache<TType>::increase_ref_count(resource_id_t id)
	{
		core::cscoped_mutex m(m_mutex);

		if (auto it = m_entries.find(id); it != m_entries.end())
		{
			++it->second.m_ref_count;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	void ccache<TType>::update(float dt)
	{
		CPU_ZONE;

		core::cscoped_mutex m(m_mutex);

		for (auto it = m_entries.begin(); it != m_entries.end();)
		{
			auto& resource = it->second;

			if (resource.m_ownership == resource_ownership_non_owning &&
				resource.m_ref_count == 0 &&
				resource.m_state != resource_state_pending)
			{
				const auto resource_type = rttr::type::get<TType>();

				log::debug("Starting to unload managed resource '{} (id={}, type={})'",
					resource.m_path.empty() ? "-/-" : resource.m_path.generic_string(),
					resource.m_id,
					resource_type.get_name().data());

				//- Destroy resource
				if (resource.m_data.has_value())
				{
					TType::unload(resource.m_data.value());
				}

				it = m_entries.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	void ccache<TType>::shutdown()
	{
		core::cscoped_mutex m(m_mutex);

		m_pending_unload.clear();

		//- Before we can unload everything, we have to wait for any pending loading tasks
		while (!m_pending_load.empty())
		{
			auto& [id, opt_data] = m_pending_load.back();
			auto& entry = m_entries[id];
			if (opt_data.has_value())
			{
				entry.m_data = std::move(opt_data);
			}
			m_pending_load.pop();
		}

		//- Unload all valid and loaded resources
		for (auto& [_, resource] : m_entries)
		{
			TType::unload(resource.m_data.value());
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
		return m_entries.at(id).m_data.value();
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TType>
	void ccache<TType>::commit()
	{
		CPU_ZONE;

		const auto resource_type = rttr::type::get<TType>();

		//- Function moves resources from pending to use-ready entries
		//- and updates its state and assigns a valid id
		core::cscoped_mutex m(m_mutex);
		while (!m_pending_load.empty())
		{
			auto& [id, opt_data] = m_pending_load.front();
			auto& entry = m_entries.find(id)->second;

			if (opt_data.has_value())
			{
				//- Note, id and path are assigned by the resource manager
				entry.m_data = std::move(opt_data);
				entry.m_state = resource_state_finished;

				log::info("Successfully loaded resource '{} (id={}, type={})'", entry.m_path.empty() ? "-/-" : entry.m_path.generic_string(),
					id, resource_type.get_name().data());
			}
			else
			{
				entry.m_state = resource_state_failed;

				log::err("Failed loading resource '{} (id={}, type={})'", entry.m_path.empty() ? "-/-" : entry.m_path.generic_string(),
					id, resource_type.get_name().data());
			}

			m_pending_load.pop();

			//- Check if an unload was scheduled for this resource, and if so, perform it here
			if (m_pending_unload.count(id) > 0)
			{
				TType::unload(entry.m_data.value());
				m_entries.erase(id);
				m_pending_unload.erase(id);
			}
		}
	}

} //- kokoro