#pragma once
#include <engine/iservice.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/io.hpp>
#include <core/hash.hpp>
#include <core/mutex.hpp>
#include <engine.hpp>
#include <fmt.h>

namespace kokoro
{
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
			filepath_t m_path;
			const TSnapshot* m_snapshot;
		};

		virtual ~iresource_manager_service() = default;

		const TSnapshot*		snapshot(filepath_t path);					//- Load or retrieve the snapshot file at given filepath
		TResource*				instantiate(filepath_t path);				//- Create a new resource instance from given snapshot
		void					destroy(TResource* instance);				//- Destroy given instance
		filepath_t				filepath(TResource* instance) const;		//- Retrieve the filepath of the snapshot from which the instance was created

	protected:
		using hashed_path_t = uint64_t;
		std::unordered_map<hashed_path_t, TSnapshot> m_snapshots;
		std::vector<sinstance> m_instances;
		mutable core::cmutex m_snapshot_mutex;
		mutable core::cmutex m_instances_mutex;

	protected:
		virtual TResource		do_instantiate(const TSnapshot*) = 0;
		virtual void			do_destroy(TResource*) = 0;

	private:
		uint64_t				hash(filepath_t path) { return core::hash(path.generic_string()); }
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	void iresource_manager_service<TResource, TSnapshot, C_UNIQUE_INSTANCES>::destroy(TResource* instance)
	{
		do_destroy(instance);

		//- Find and erase entry from instances
		{
			auto* ptr = reinterpret_cast<sinstance*>(instance);

			core::cscoped_mutex m(m_instances_mutex);

			for (auto i = 0; i < m_instances.size(); ++i)
			{
				const auto& inst = m_instances[i];

				if (inst.m_path == ptr->m_path)
				{
					m_instances.erase(m_instances.begin() + i);
					return;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	filepath_t iresource_manager_service<TResource, TSnapshot, C_UNIQUE_INSTANCES>::filepath(TResource* instance) const
	{
		auto* ptr = reinterpret_cast<sinstance*>(instance);
		return ptr->m_path;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TResource, typename TSnapshot, bool C_UNIQUE_INSTANCES /*= true*/>
	TResource* iresource_manager_service<TResource, TSnapshot, C_UNIQUE_INSTANCES>::instantiate(filepath_t path)
	{
		TResource* output = nullptr;

		//- If we have non-unique instances, meaning there is only one instance or none,
		//- then we try to resolve and return it if it exists before creating a new one
		if constexpr (!C_UNIQUE_INSTANCES)
		{
			core::cscoped_mutex m(m_instances_mutex);

			for (const auto& inst : m_instances)
			{
				if (inst.m_path == path)
				{
					return reinterpret_cast<TResource*>(&inst);
				}
			}
		}

		//- Instantiate the resource
		{
			const auto* snaps = snapshot(path);

			core::cscoped_mutex m(m_instances_mutex);

			auto& inst = m_instances.emplace_back(std::move(do_instantiate(snaps)));
			inst.m_path = path;
			inst.m_snapshot = snaps;

			return reinterpret_cast<TResource*>(&inst);
		}
		return nullptr;
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

		if (const auto it = m_snapshots.find(path); it != m_snapshots.end())
		{
			return &it->second;
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
					path).c_str());
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
						std::forward_as_tuple(path),
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