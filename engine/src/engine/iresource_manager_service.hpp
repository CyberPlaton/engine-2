#pragma once
#include <engine/iservice.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/io.hpp>
#include <core/hash.hpp>
#include <core/mutex.hpp>
#include <engine.hpp>
#include <fmt.h>
#include <queue>

namespace kokoro
{
	using resource_handle_t = uint64_t;
#define invalid_handle_t std::numeric_limits<resource_handle_t>::max()

	static bool is_valid(resource_handle_t handle) { return handle != invalid_handle_t; }

	//- Responsible for loading snapshots of certain resource types and instantiating them, i.e. creating a runtime object.
	//- API access provided through filepath of the desired resource, depending on the configuration either a unique instance
	//- is created or access to an already existing one is provided.
	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES = true>
	class iresource_manager_service : public iservice
	{
	public:
		struct sinstance : public TResource
		{
			sinstance(TResource&& resource) : TResource(std::move(resource)) {}

			filepath_t m_path;
			const TSnapshot* m_snapshot = nullptr;
		};

		virtual ~iresource_manager_service() = default;

		const TSnapshot*		snapshot(filepath_t path);					//- Load or retrieve the snapshot file at given filepath
		resource_handle_t		instantiate(filepath_t path);				//- Create a new resource instance from given snapshot
		void					destroy(resource_handle_t handle);			//- Destroy given instance
		filepath_t				filepath(resource_handle_t handle) const;	//- Retrieve the filepath of the snapshot from which the instance was created
		TResource*				get(resource_handle_t handle);
		const TResource*		find(resource_handle_t handle) const;

	protected:
		using hashed_path_t = uint64_t;

		mutable core::cmutex m_snapshot_mutex;
		mutable core::cmutex m_instances_mutex;
		std::unordered_map<hashed_path_t, TSnapshot> m_snapshots;
		std::vector<sinstance> m_instances;
		std::queue<resource_handle_t> m_free_handles;

	protected:
		virtual TResource		do_instantiate(const TSnapshot*) = 0;
		virtual void			do_destroy(TResource*) = 0;

	private:
		uint64_t				hash(filepath_t path) { return core::hash(path.generic_string()); }
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	const TResource* iresource_manager_service<TResource, TSnapshot, C_UNIQUE_INSTANCES>::find(resource_handle_t handle) const
	{
		core::cscoped_mutex m(m_instances_mutex);

		if (handle > m_instances.size())
		{
			return nullptr;
		}
		return reinterpret_cast<const TResource*>(&m_instances[handle]);
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	TResource* iresource_manager_service<TResource, TSnapshot, C_UNIQUE_INSTANCES>::get(resource_handle_t handle)
	{
		core::cscoped_mutex m(m_instances_mutex);

		if (handle > m_instances.size())
		{
			return nullptr;
		}
		return reinterpret_cast<TResource*>(&m_instances[handle]);
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	void iresource_manager_service<TResource, TSnapshot, C_UNIQUE_INSTANCES>::destroy(resource_handle_t handle)
	{
		if (auto* inst = get(handle); inst)
		{
			core::cscoped_mutex m(m_instances_mutex);

			do_destroy(inst);
		}

		core::cscoped_mutex m(m_instances_mutex);

		//- Find and erase entry from instances
		const auto it = m_instances.begin() + (handle - static_cast<resource_handle_t>(m_free_handles.size()));
		m_instances.erase(it);
		m_free_handles.push(handle);
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	filepath_t iresource_manager_service<TResource, TSnapshot, C_UNIQUE_INSTANCES>::filepath(resource_handle_t handle) const
	{
		if (const auto* inst = find(handle); inst)
		{
			core::cscoped_mutex m(m_instances_mutex);

			const auto* ptr = reinterpret_cast<const sinstance*>(inst);
			return ptr->m_path;
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	resource_handle_t iresource_manager_service<TResource, TSnapshot, C_UNIQUE_INSTANCES>::instantiate(filepath_t path)
	{
		resource_handle_t output = invalid_handle_t;

		//- If we have non-unique instances, meaning there is only one instance or none,
		//- then we try to resolve and return it if it exists before creating a new one
		if constexpr (!C_UNIQUE_INSTANCES)
		{
			core::cscoped_mutex m(m_instances_mutex);

			for (auto i = 0; i < m_instances.size(); ++i)
			{
				const auto& inst = m_instances[i];

				if (inst.m_path == path)
				{
					return static_cast<resource_handle_t>(i);
				}
			}
		}

		//- Instantiate the resource
		const auto* snaps = snapshot(path);

		core::cscoped_mutex m(m_instances_mutex);

		//- Either reuse free indices or emplace a new entry
		if (!m_free_handles.empty())
		{
			auto idx = m_free_handles.back();
			m_free_handles.pop();

			//- If the index is higher than current instances count, then we just emplace a new one
			//- and ignore the free handle
			if (idx < m_instances.size())
			{
				m_instances[idx] = std::move(do_instantiate(snaps));
				m_instances[idx].m_path = path;
				m_instances[idx].m_snapshot = snaps;
				output = static_cast<resource_handle_t>(idx);
			}
			else
			{
				auto& inst = m_instances.emplace_back(std::move(do_instantiate(snaps)));
				inst.m_path = path;
				inst.m_snapshot = snaps;
				output = static_cast<resource_handle_t>(m_instances.size() - 1);
			}
		}
		else
		{
			auto& inst = m_instances.emplace_back(std::move(do_instantiate(snaps)));
			inst.m_path = path;
			inst.m_snapshot = snaps;
			output = static_cast<resource_handle_t>(m_instances.size() - 1);
		}

		return output;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	const TSnapshot* iresource_manager_service<TResource, TSnapshot, C_UNIQUE_INSTANCES>::snapshot(filepath_t path)
	{
		//- Try to resolve the snapshot before loading and deserializing
		{
			core::cscoped_mutex m(m_snapshot_mutex);

			if (const auto it = m_snapshots.find(hash(path)); it != m_snapshots.end())
			{
				return &it->second;
			}
		}

		auto& vfs = instance().service<cvirtual_filesystem_service>();
		filepath_t p(path);

		if (!vfs.exists(p))
		{
			//- Try resolving filepath using virtual file system
			if (const auto [result, value] = vfs.resolve(p); result)
			{
				p = value;
			}
			else
			{
				instance().service<clog_service>().err(fmt::format("Could not find resource snapshot file at '{}'",
					path.generic_string()).c_str());
				return nullptr;
			}
		}

		if (auto file = vfs.open(p, file_options_read | file_options_text); file)
		{
			auto future = file->read_async();

			while (future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::ready) {}
			file->close();

			auto mem = future.get();

			if (mem && !mem->empty())
			{
				if (auto var = core::from_json_blob(rttr::type::get<TSnapshot>(), mem->data(), mem->size()); var.is_valid())
				{
					core::cscoped_mutex m(m_snapshot_mutex);

					if (auto [it, result] = m_snapshots.emplace(std::piecewise_construct,
						std::forward_as_tuple(hash(path)),
						std::forward_as_tuple(std::move(var.get_value<TSnapshot>()))); result)
					{
						return &it->second;
					}
				}
			}
		}
		return nullptr;
	}

} //- kokoro