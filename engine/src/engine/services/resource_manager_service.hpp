#pragma once
#include <engine/iservice.hpp>
#include <engine/resource.hpp>
#include <engine/services/thread_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/hash.hpp>
#include <core/io.hpp>
#include <engine.hpp>
#include <fmt.h>
#include <rttr.h>
#include <atomic>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class cresource_manager_service final : public iservice
	{
	public:
		cresource_manager_service() = default;
		~cresource_manager_service() override = default;

		template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES = true>
		cresource_manager_service&		new_manager();
		bool							init();
		void							post_init() {}
		void							shutdown();
		void							update(float);
		template<typename TResource>
		cview<TResource>				load(filepath_t path);

		template<typename TResource, typename TSnapshot>
		cview<TResource>				load_from_snapshot(const TSnapshot& snapshot);

		template<typename TResource>
		void							unload(resource_id_t id);

		template<typename TResource>
		rttr::variant					snapshot(filepath_t path);

	private:
		struct sresource_cache_desc
		{
			sresource_cache_desc(std::unique_ptr<icache>&& cache, rttr::type resource_type, rttr::type snapshot_type, uint64_t next_id, bool unique_instances);
			sresource_cache_desc(sresource_cache_desc&& other) noexcept;
			sresource_cache_desc& operator=(sresource_cache_desc&&) = delete;
			sresource_cache_desc(const sresource_cache_desc&) = delete;
			sresource_cache_desc& operator=(const sresource_cache_desc&) = delete;

			std::unique_ptr<icache> m_cache = nullptr;
			rttr::type m_resource_type;
			rttr::type m_snapshot_type;
			resource_id_t m_next_id = 0;
			const bool m_unique_instances = true;
		};

		mutable core::cmutex m_mutex;
		std::unordered_map<resource_type_id_t, sresource_cache_desc> m_cache_descs;
		std::unordered_map<hashed_path_t, rttr::variant> m_snapshots;

	private:
		const sresource_cache_desc&		cache_desc(resource_type_id_t type_id) const;
		template<typename TResource>
		ccache<TResource>*				cache();
		template<typename TResource>
		uint64_t						acquire_id();
	};

	//- Create a new manager type for resource and snapshot types. Not thread-safe and should be done once on start up.
	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	cresource_manager_service& cresource_manager_service::new_manager()
	{
		static_assert(std::is_move_constructible_v<TResource> && std::is_move_assignable_v<TResource>,
			"Resource type must be move-constructible and move-assignable");

		const auto resource_type = rttr::type::get<TResource>();
		const auto snapshot_type = rttr::type::get<TSnapshot>();
		const auto type_id = resource_type.get_id();

		sresource_cache_desc desc
		{
			std::move(std::make_unique<ccache<TResource>>()),
			resource_type,
			snapshot_type,
			0,
			C_UNIQUE_INSTANCES
		};

		if (auto [it, result] = m_cache_descs.emplace(std::piecewise_construct,
			std::forward_as_tuple(type_id),
			std::forward_as_tuple(std::move(desc))); result)
		{
		}
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource>
	rttr::variant cresource_manager_service::snapshot(filepath_t path)
	{
		CPU_ZONE;

		//- Try resolving filepath using virtual file system
		auto& vfs = instance().service<cvirtual_filesystem_service>();
		if (!vfs.exists(path))
		{
			if (const auto [result, value] = vfs.resolve(path); result)
			{
				path = value;
			}
		}

		const auto resource_type = rttr::type::get<TResource>();
		const auto type_id = resource_type.get_id();
		const auto& desc = cache_desc(type_id);
		const auto& snapshot_type = desc.m_snapshot_type;
		const auto id = core::hash(path.generic_string());

		{
			core::cscoped_mutex m(m_mutex);

			//- Try to resolve the snapshot before loading and deserializing
			if (const auto it = m_snapshots.find(id); it != m_snapshots.end())
			{
				return it->second;
			}

			if (auto file = vfs.open(path, file_options_read | file_options_text); file.opened())
			{
				auto mem = file.read_sync();
				file.close();

				if (mem && !mem->empty())
				{
					if (auto var = core::from_json_blob(snapshot_type, mem->data(), mem->size()); var.is_valid())
					{
						auto [it, result] = m_snapshots.emplace(std::piecewise_construct,
							std::forward_as_tuple(id),
							std::forward_as_tuple(var));

						return it->second;
					}
				}
			}
			else
			{
				log::err("Failed opening '{}' snapshot file at '{}'", snapshot_type.get_name().data(), path.generic_string());
			}
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource>
	ccache<TResource>* cresource_manager_service::cache()
	{
		const auto type_id = rttr::type::get<TResource>().get_id();
		return reinterpret_cast<ccache<TResource>*>(m_cache_descs.at(type_id).m_cache.get());
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource>
	uint64_t cresource_manager_service::acquire_id()
	{
		const auto type_id = rttr::type::get<TResource>().get_id();
		auto& desc = m_cache_descs.at(type_id);

		auto* c = cache<TResource>();
		core::cscoped_mutex m(c->m_mutex);
		return ++desc.m_next_id;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource>
	void cresource_manager_service::unload(resource_id_t id)
	{
		CPU_ZONE;

		auto* c = cache<TResource>();

		core::cscoped_mutex m(c->m_mutex);

		if (const auto it = c->m_entries.find(id); it != c->m_entries.end())
		{
			auto& resource = it->second;
			if (resource.m_state == resource_state_pending)
			{
				//- The resource is still being loaded, we have to wait before we can unload it
				c->m_pending_unload.insert(resource.m_id);
				return;
			}

			TResource::unload(resource.m_data.value());
			c->m_entries.erase(it);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource>
	cview<TResource> cresource_manager_service::load(filepath_t path)
	{
		CPU_ZONE;

		const auto id = acquire_id<TResource>();
		const auto resource_type = rttr::type::get<TResource>();
		const auto type_id = resource_type.get_id();
		const auto& desc = cache_desc(type_id);

		log::debug("Starting to load resource '{} (id={}, type={})'", path.generic_string(), id, resource_type.get_name().data());

		auto* c = cache<TResource>();

		//- When we are not having unique instances, we want to check for existing resource and return it instead of loading anew
		if(!desc.m_unique_instances)
		{
			core::cscoped_mutex m(c->m_mutex);

			if (const auto it = c->m_entries.find(id); it != c->m_entries.end())
			{
				return cview<TResource>(id, c);
			}
		}

		//- Immediately insert pending entry
		{
			core::cscoped_mutex m(c->m_mutex);
			auto& entry = c->m_entries[id];
			entry.m_id = id;
			entry.m_path = path;
			entry.m_state = resource_state_pending;
		}

		rttr::variant snaps = snapshot<TResource>(path);

		//- Create a task for loading the resource
		instance().service<cthread_service>().async(fmt::format("load at path '{}'", path.generic_u8string()),
			[c, id, snaps=std::move(snaps), path=path, resource_type= resource_type]()
			{
				//- Perform the actual loading of the resource. Success indicates whether the loading
				//- was in order and we can proceed storing the resource
				auto opt_data = TResource::load(snaps);

				if (!opt_data.has_value())
				{
					log::err("Failed loading resource '{} (id={}, type={})'", path.generic_string(), id, resource_type.get_name().data());
				}

				core::cscoped_mutex m(c->m_mutex);
				c->m_pending_load.push({ id, std::move(opt_data) });
			});

		return cview<TResource>(id, c);
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot>
	cview<TResource> cresource_manager_service::load_from_snapshot(const TSnapshot& snapshot)
	{
		CPU_ZONE;

		const auto id = acquire_id<TResource>();
		const auto resource_type = rttr::type::get<TResource>();
		const auto type_id = resource_type.get_id();
		const auto& desc = cache_desc(type_id);

		log::debug("Starting to load resource from snapshot '(id={}, type={})'", id, resource_type.get_name().data());

		auto* c = cache<TResource>();

		//- When we are not having unique instances, we want to check for existing resource and return it instead of loading anew
		if (!desc.m_unique_instances)
		{
			core::cscoped_mutex m(c->m_mutex);

			if (const auto it = c->m_entries.find(id); it != c->m_entries.end())
			{
				return cview<TResource>(id, c);
			}
		}

		//- Immediately insert pending entry
		{
			core::cscoped_mutex m(c->m_mutex);
			auto& entry = c->m_entries[id];
			entry.m_id = id;
			entry.m_state = resource_state_pending;
		}

		//- Create a task for loading the resource
		instance().service<cthread_service>().async(fmt::format("load from snapshot '{}'", id),
			[c, id, snaps = std::move(rttr::variant(snapshot)), resource_type = resource_type]()
			{
				//- Perform the actual loading of the resource. Success indicates whether the loading
				//- was in order and we can proceed storing the resource
				auto opt_data = TResource::load(snaps);

				if (!opt_data.has_value())
				{
					log::err("Failed loading resource from snapshot '(id={}, type={})'", id, resource_type.get_name().data());
				}

				core::cscoped_mutex m(c->m_mutex);
				c->m_pending_load.push({ id, std::move(opt_data) });
			});

		return cview<TResource>(id, c);
	}

} //- kokoro