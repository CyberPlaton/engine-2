#include <engine/world/managers/query_manager.hpp>
#include <engine/components/common.hpp>
#include <engine/components/sprite.hpp>
#include <engine/components/transforms.hpp>
#include <engine/math/ray.hpp>
#include <core/hash.hpp>

namespace kokoro
{
	namespace detail
	{
		//------------------------------------------------------------------------------------------------------------------------
		bool query_callback(int proxy_id, int user_data, void* ctx)
		{
			auto* qm = reinterpret_cast<cquery_manager*>(ctx);
			bool result = false;

			switch (qm->m_current_query.m_type)
			{
			case squery::type_count: { result = true; break; }
			case squery::type_all: { result = true; break; }
			case squery::type_any: { result = false; break; }
			default:
			case squery::type_none: return false;
			}

			//- Assign query key to proxy and avaoid duplicate queries
			auto& proxy = qm->proxy(user_data/*box2d::b2DynamicTree_GetUserData(&pm.tree(), proxy_id)*/);

			if (proxy.m_query_key == qm->m_current_query_key)
			{
				return result;
			}

			proxy.m_query_key = qm->m_current_query_key;

			//- TODO: apply filters

			switch (qm->m_current_query.m_type)
			{
			case squery::type_count:
			{
				auto& var = qm->m_query_results[qm->m_current_query_key];
				auto& value = var.get_value<uint64_t>(); ++value;
				break;
			}
			case squery::type_all:
			{
				auto& var = qm->m_query_results[qm->m_current_query_key];
				auto& value = var.get_value<std::vector<flecs::entity>>();
				value.push_back(proxy.m_entity);
				break;
			}
			case squery::type_any:
			{
				auto& var = qm->m_query_results[qm->m_current_query_key];
				auto& value = var.get_value<bool>();
				value = true;
				break;
			}
			default:
			case squery::type_none: return false;
			}

			return result;
		}

		//------------------------------------------------------------------------------------------------------------------------
		float raycast_callback(const box2d::b2RayCastInput* input, int proxy_id, int user_data, void* ctx)
		{
			//- 0.0f signals to stop and 1.0f signals to continue
			auto* qm = reinterpret_cast<cquery_manager*>(ctx);
			float result = 0.0f;

			switch (qm->m_current_query.m_type)
			{
			case squery::type_count: { result = 1.0f; break; }
			case squery::type_all: { result = 1.0f; break; }
			case squery::type_any: { result = 0.0f; break; }
			default:
			case squery::type_none: return false;
			}

			//- Assign query key to proxy and avaoid duplicate queries
			auto& proxy = qm->proxy(user_data/*box2d::b2DynamicTree_GetUserData(&pm.tree(), proxy_id)*/);

			if (proxy.m_query_key == qm->m_current_query_key)
			{
				return result;
			}

			proxy.m_query_key = qm->m_current_query_key;

			//- TODO: apply filters

			switch (qm->m_current_query.m_type)
			{
			case squery::type_count:
			{
				auto& var = qm->m_query_results[qm->m_current_query_key];
				auto& value = var.get_value<uint64_t>(); ++value;
				break;
			}
			case squery::type_all:
			{
				auto& var = qm->m_query_results[qm->m_current_query_key];
				auto& value = var.get_value<std::vector<flecs::entity>>();
				value.emplace_back(proxy.m_entity);
				break;
			}
			case squery::type_any:
			{
				auto& var = qm->m_query_results[qm->m_current_query_key];
				auto& value = var.get_value<bool>();
				value = true;
				break;
			}
			default:
			case squery::type_none: return false;
			}

			return result;
		}

	} //- detail

	//------------------------------------------------------------------------------------------------------------------------
	void cquery_manager::init()
	{
		m_quad_tree = box2d::b2DynamicTree_Create();

		m_proxy_observer = m_world
			->observer<world::component::ssprite, world::component::slocal_transform, world::component::sidentifier>()
			.event(flecs::OnAdd)
			.event(flecs::OnRemove)
			.ctx(this)
			.each([](flecs::iter& it, uint64_t i,
				world::component::ssprite&,
				world::component::slocal_transform&,
				world::component::sidentifier& id)
				{
					if (auto* manager = reinterpret_cast<cquery_manager*>(it.ctx()); manager)
					{
						auto e = it.entity(i);
						const auto has_proxy = manager->has_proxy(e);

						//- Observe when entities enter or leave the archetype (sprite, transform and id)
						//- and manage their internal dynamic tree proxy
						if (it.event() == flecs::OnAdd && !has_proxy)
						{
							manager->queue_create_proxy(e);
						}
						else if (it.event() == flecs::OnRemove && has_proxy)
						{
							manager->destroy_proxy(e);
						}
					}
				});
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cquery_manager::shutdown()
	{
		m_proxy_observer.destruct();
		box2d::b2DynamicTree_Destroy(&m_quad_tree);
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cquery_manager::query_create(squery::type type, squery::intersection intersection, squery::data_t data, squery::filters_t filters /*= {}*/) -> query_handle_t
	{
		query_handle_t i = m_next_query_key;
		m_next_query_key = next_query_key();

		squery q;
		q.m_type = type;
		q.m_intersection = intersection;
		q.m_data = data;
		q.m_filters = filters;

		m_queries.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(i),
			std::forward_as_tuple(std::move(q)));

		return i;
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cquery_manager::query_immediate(squery::type type, squery::intersection intersection, squery::data_t data, squery::filters_t filters /*= {}*/) -> squery::result_t
	{
		squery q;
		q.m_type = type;
		q.m_intersection = intersection;
		q.m_data = data;
		q.m_filters = filters;

		query_handle_t i = m_next_query_key;
		m_next_query_key = next_query_key();

		m_current_query_key = i;
		m_current_query = q;
		m_query_results[i] = result_type(q.m_type);

		do_query(q);

		//- Get the result for return and erase it from result map
		const auto it = m_query_results.find(i);
		rttr::variant var = std::move(it->second);
		m_query_results.erase(it);
		return var;
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cquery_manager::query_result(query_handle_t id) -> std::optional<squery::result_t>
	{
		std::optional<squery::result_t> out = std::nullopt;

		if (const auto it = m_query_results.find(id); it != m_query_results.end())
		{
			out = it->second;

			//- Result will be taken and we do not need to hold it anymore
			m_query_results.erase(it);
		}
		return out;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cquery_manager::pre_tick()
	{
		//- Process proxy creation requests
		for (const auto& e : m_creation_queue)
		{
			create_proxy(e);
		}
		m_creation_queue.clear();

		//- Process change trackers reacting to world changes from previous frame
		for (auto& [_, tracker] : m_change_trackers)
		{
			tracker->tick();
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cquery_manager::post_tick()
	{
		//- Clear previous results so that we only contain fresh ones
		m_query_results.clear();

		for (auto& [key, cfg] : m_queries)
		{
			//- Prepare query and let it run
			m_current_query_key = key;
			m_current_query = cfg;
			do_query(m_current_query);
		}

		//- Clear queries we have processed so that we only contain relevant ones
		m_queries.clear();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cquery_manager::update_proxy(flecs::entity e)
	{
		if (const auto* id = e.get<world::component::sidentifier>(); id)
		{
			if (const auto it = m_proxies.find(id->m_uuid); it != m_proxies.end())
			{
				auto& proxy = m_proxies[id->m_uuid];
				const auto* transform = e.get<world::component::sworld_transform>();
				proxy.m_aabb = transform->m_aabb;
				box2d::b2DynamicTree_MoveProxy(&m_quad_tree, proxy.m_id, transform->m_aabb);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cquery_manager::destroy_proxy(flecs::entity e)
	{
		if (const auto* id = e.get<world::component::sidentifier>(); id)
		{
			if (const auto it = m_proxies.find(id->m_uuid); it != m_proxies.end())
			{
				box2d::b2DynamicTree_DestroyProxy(&m_quad_tree, it->second.m_id);
				m_proxies.erase(it);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cquery_manager::create_proxy(flecs::entity e)
	{
		const auto* id = e.get<world::component::sidentifier>();
		const auto* transform = e.get<world::component::sworld_transform>();
		const auto data_id = user_data_id(e);
		auto& proxy = m_proxies[id->m_uuid];
		proxy.m_aabb = transform->m_aabb;
		proxy.m_entity = e;
		proxy.m_id = box2d::b2DynamicTree_CreateProxy(&m_quad_tree, transform->m_aabb, B2_DEFAULT_CATEGORY_BITS, data_id);

		m_user_data[data_id] = id->m_uuid;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cquery_manager::queue_create_proxy(flecs::entity e)
	{
		m_creation_queue.push_back(e);
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cquery_manager::has_proxy(flecs::entity e) const
	{
		if (const auto* id = e.get<world::component::sidentifier>(); id)
		{
			return m_proxies.find(id->m_uuid) != m_proxies.end();
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	cquery_manager::sproxy& cquery_manager::proxy(int proxy_id)
	{
		if (const auto it = m_user_data.find(proxy_id); it != m_user_data.end())
		{
			if (const auto p = m_proxies.find(it->second); p != m_proxies.end())
			{
				return p->second;
			}
		}

		static sproxy S_DUMMY;
		return S_DUMMY;
	}

	//------------------------------------------------------------------------------------------------------------------------
	int cquery_manager::user_data_id(const flecs::entity& e) const
	{
		return core::hash(e.get<world::component::sidentifier>()->m_uuid.string());
	}

	//------------------------------------------------------------------------------------------------------------------------
	cquery_manager::query_handle_t cquery_manager::next_query_key()
	{
		return (m_next_query_key + 1) % C_MASTER_QUERY_KEY_MAX;
	}

	//------------------------------------------------------------------------------------------------------------------------
	squery::result_t cquery_manager::result_type(squery::type t)
	{
		switch (t)
		{
		case squery::type_count:	return rttr::variant(static_cast<uint64_t>(0));
		case squery::type_any:		return rttr::variant(false);
		case squery::type_all:		return rttr::variant(std::vector<flecs::entity>{});
		default:
		case squery::type_none:		return {};
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cquery_manager::do_query(const squery& q)
	{
		m_query_results[m_current_query_key] = result_type(q.m_type);
		auto& tree = m_quad_tree;

		if (m_current_query.m_intersection == squery::intersection_aabb &&
			q.m_data.get_type() == rttr::type::get<math::aabb_t>())
		{
			box2d::b2DynamicTree_Query(&tree, q.m_data.get_value<math::aabb_t>(), B2_DEFAULT_MASK_BITS, &detail::query_callback, this);
		}

		else if (m_current_query.m_intersection == squery::intersection_ray &&
			q.m_data.get_type() == rttr::type::get<math::ray_t>())
		{
			box2d::b2RayCastInput ray = q.m_data.get_value<math::ray_t>();

			box2d::b2DynamicTree_RayCast(&tree, &ray, B2_DEFAULT_MASK_BITS, &detail::raycast_callback, this);
		}
	}

} //- kokoro