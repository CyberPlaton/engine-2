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
		template<typename TResource>
		void							unload(filepath_t path);
		template<typename TResource>
		rttr::variant					snapshot(filepath_t path);

	private:
		struct sresource_cache_desc
		{
			std::unique_ptr<icache> m_cache = nullptr;
			rttr::type m_resource_type;
			rttr::type m_snapshot_type;
			const bool m_unique_instances = true;
		};

		mutable core::cmutex m_mutex;
		std::unordered_map<resource_type_id_t, sresource_cache_desc> m_cache_descs;
		std::unordered_map<hashed_path_t, rttr::variant> m_snapshots;

	private:
		const sresource_cache_desc&		cache_desc(resource_type_id_t type_id) const;
		template<typename TResource>
		ccache<TResource>*				cache();
	};

	//- Create a new manager type for resource and snapshot types. Not thread-safe and should be done once on start up.
	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	cresource_manager_service& cresource_manager_service::new_manager()
	{
		static_assert(std::is_copy_constructible_v<TResource> && std::is_copy_constructible_v<TSnapshot>,
			"Resource and snapshot types must be copy-constructible");

		const auto resource_type = rttr::type::get<TResource>();
		const auto snapshot_type = rttr::type::get<TSnapshot>();
		const auto type_id = resource_type.get_id();

		sresource_cache_desc desc
		{
			std::make_unique<ccache<TResource>>(),
			resource_type,
			snapshot_type,
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
		//- Try resolving filepath using virtual file system
		auto& vfs = instance().service<cvirtual_filesystem_service>();
		if (!vfs.exists(path))
		{
			if (const auto [result, value] = vfs.resolve(path); result)
			{
				path = value;
			}
			else
			{
				instance().service<clog_service>().err(fmt::format("Could not find resource snapshot file at '{}'", path.generic_string()).c_str());
			}
		}

		//- Try to resolve the snapshot before loading and deserializing
		{
			core::cscoped_mutex m(m_mutex);

			if (const auto it = m_snapshots.find(core::hash(path.generic_string())); it != m_snapshots.end())
			{
				return it->second;
			}
		}

		if (auto file = vfs.open(path, file_options_read | file_options_text); file.opened())
		{
			auto mem = file.read_sync();

			if (mem && !mem->empty())
			{
				const auto resource_type = rttr::type::get<TResource>();
				const auto type_id = resource_type.get_id();

				if (auto var = core::from_json_blob(m_cache_descs.at(type_id).m_snapshot_type, mem->data(), mem->size()); var.is_valid())
				{
					core::cscoped_mutex m(m_mutex);

					auto [it, result] = m_snapshots.emplace(std::piecewise_construct,
						std::forward_as_tuple(core::hash(path.generic_string())),
						std::forward_as_tuple(var));

					return it->second;
				}
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
	void cresource_manager_service::unload(filepath_t path)
	{
		//- FIXME: something else for IDs needed, otherwise we cant have multiple instances from same file
		const auto id = core::hash(path.generic_string());
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

			TResource::unload(resource.m_data);
			c->m_entries.erase(it);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource>
	cview<TResource> cresource_manager_service::load(filepath_t path)
	{
		//- FIXME: something else for IDs needed, otherwise we cant have multiple instances from same file
		const auto id = core::hash(path.generic_string());
		const auto resource_type = rttr::type::get<TResource>();
		const auto type_id = resource_type.get_id();
		const auto& desc = cache_desc(type_id);

		instance().service<clog_service>().debug(fmt::format("Loading resource '{} (id={}, type={})'",
			path.generic_string(),
			id,
			resource_type.get_name().data()).c_str());

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

			instance().service<clog_service>().debug(fmt::format("Inserting pending resource entry '{} (id={}, type={})'",
				path.generic_string(),
				id,
				resource_type.get_name().data()).c_str());
		}

		rttr::variant snaps = snapshot<TResource>(path);

		//- Create a task for loading the resource
		instance().service<cthread_service>().async(fmt::format("load '{}'", path.generic_u8string()),
			[c, id, snaps=std::move(snaps), path=path, resource_type= resource_type]()
			{
				//- Perform the actual loading of the resource. Success indicates whether the loading
				//- was in order and we can proceed storing the resource
				auto [success, data] = TResource::load(snaps);

				core::cscoped_mutex m(c->m_mutex);
				c->m_pending_load.push({ success, id, std::move(data) });

				instance().service<clog_service>().debug(fmt::format("Resource moved to pending load '{} (id={}, type={})'",
					path.generic_string(),
					id,
					resource_type.get_name().data()).c_str());
			});

		return cview<TResource>(id, c);
	}

} //- kokoro