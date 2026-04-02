#pragma once
#include <engine/world/change_tracker.hpp>
#include <engine/math/aabb.hpp>
#include <core/uuid.hpp>
#include <flecs.h>
#include <box2d.h>
#include <rttr.h>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <optional>

namespace kokoro
{
	namespace detail
	{
		bool query_callback(int proxy_id, int user_data, void* ctx);
		float raycast_callback(const box2d::b2RayCastInput* input, int proxy_id, int user_data, void* ctx);

		//------------------------------------------------------------------------------------------------------------------------
		constexpr uint64_t fnv1a(uint64_t hash, uint64_t value)
		{
			return (hash ^ value) * 1099511628211ull;
		}

		//------------------------------------------------------------------------------------------------------------------------
		template<typename TType>
		constexpr uint64_t type_id()
		{
			return reinterpret_cast<uint64_t>(&type_id<TType>);
		}

		//- Function used to create a unique hash of a components parameter pack.
		//------------------------------------------------------------------------------------------------------------------------
		template<typename... TComps>
		constexpr uint64_t comps_hash()
		{
			uint64_t h = 14695981039346656037ull;
			((h = fnv1a(h, type_id<TComps>())), ...);
			return h;
		}

	} //- detail

	//- Representing a query into the world.
	//------------------------------------------------------------------------------------------------------------------------
	struct squery final
	{
		using data_t = rttr::variant;					//- Depending on intersection holding either a ray or an aabb
		using filters_t = std::vector<rttr::variant>;	//- Array of sfilter variants
		using result_t = rttr::variant;					//- Depending on the query type holding:

		enum type : uint8_t
		{
			type_none = 0,
			type_any,		//- Return whether we encountered any entity, bool
			type_all,		//- Return all encountered entities, std::vector< flecs::entity >
			type_count,		//- Return the number of encountered entities, uint64_t
		};

		enum intersection : uint8_t
		{
			intersection_none = 0,
			intersection_ray,		//- Querying by raycast
			intersection_aabb,		//- Querying by aabb
		};

		filters_t m_filters;
		data_t m_data;
		type m_type = type_none;
		intersection m_intersection = intersection_none;
	};

	//------------------------------------------------------------------------------------------------------------------------
	class cquery_manager final
	{
		friend bool detail::query_callback(int proxy_id, int user_data, void* ctx);
		friend float detail::raycast_callback(const box2d::b2RayCastInput* input, int proxy_id, int user_data, void* ctx);
	public:
		using entity_proxy_t = int;
		using query_handle_t = uint64_t;
		using entities_t = std::vector<flecs::entity>;
		using results_t = std::unordered_map<query_handle_t, squery::result_t>;
		using queries_t = std::map<query_handle_t, squery>;
		using change_trackers_t = std::unordered_map<uint64_t, std::shared_ptr<world::ichange_tracker>>;

		static inline constexpr auto C_MASTER_QUERY_KEY_MAX = 128;
		static inline constexpr auto C_INVALID_QUERY_ID = std::numeric_limits<query_handle_t>().max();

		struct sproxy final
		{
			math::aabb_t m_aabb;
			flecs::entity m_entity;
			entity_proxy_t m_id = std::numeric_limits<entity_proxy_t>().max();
			unsigned m_query_key = 0;
		};

		using proxy_map_t = std::unordered_map<core::cuuid, sproxy>;
		using user_data_map_t = std::unordered_map<int, core::cuuid>;

		cquery_manager() = default;
		~cquery_manager() = default;

		void	world(flecs::world* w) { m_world = w; };
		void	init();
		void	shutdown();

		auto	query_create(squery::type type, squery::intersection intersection, squery::data_t data, squery::filters_t filters = {}) -> query_handle_t;
		auto	query_immediate(squery::type type, squery::intersection intersection, squery::data_t data, squery::filters_t filters = {}) -> squery::result_t;
		auto	query_result(query_handle_t id) -> std::optional<squery::result_t>;

		template<typename... TComps>
		auto	change_tracker() -> world::change_tracker_t;

		template<typename... TComps, typename TCallable>
		auto	query_one(TCallable callback) -> flecs::entity;

		template<typename... TComps, typename TCallable>
		auto	query_all(TCallable callback) -> entities_t;

		void	pre_tick();
		void	post_tick();
		void	update_proxy(flecs::entity e);
		void	destroy_proxy(flecs::entity e);
		void	create_proxy(flecs::entity e);
		void	queue_create_proxy(flecs::entity e);
		bool	has_proxy(flecs::entity e) const;
		sproxy&	proxy(int proxy_id);
		auto	tree() -> box2d::b2DynamicTree& { return m_quad_tree; }
		int		user_data_id(const flecs::entity& e) const;

	private:
		flecs::world* m_world = nullptr;
		proxy_map_t m_proxies;
		user_data_map_t m_user_data;
		box2d::b2DynamicTree m_quad_tree;
		std::vector<flecs::entity> m_creation_queue;
		change_trackers_t m_change_trackers;			//- All component archetype change trackers created for world
		results_t m_query_results;						//- Containing results of queries
		squery m_current_query;							//- Currently executed query
		queries_t m_queries;							//- All queries to be executed in order of their id, this way we maintain order and fast lookup
		query_handle_t m_current_query_key = 0;			//- Id of the current query
		query_handle_t m_next_query_key = 0;			//- Id to be assigned to the next created query
		flecs::observer m_proxy_observer;				//- Observer for creation of archetypes to create proxies

	private:
		query_handle_t next_query_key();
		squery::result_t result_type(squery::type t);
		void do_query(const squery& q);
	};

	//- Create a component change tracker for a world. The tracker triggers once archetypes were modified
	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps>
	auto cquery_manager::change_tracker() -> world::change_tracker_t
	{
		const auto h = detail::comps_hash<TComps...>();

		if (const auto it = m_change_trackers.find(h); it == m_change_trackers.end())
		{
			auto [_, success] = m_change_trackers.emplace(std::piecewise_construct,
				std::forward_as_tuple(h),
				std::forward_as_tuple(std::move(std::make_shared<world::cchange_tracker<TComps...>>(*m_world))));

			if (!success)
			{
				return {};
			}
		}

		return m_change_trackers[h];
	}

	//- Ex.:
	//- query::one<ecs::stransform>([](const ecs::stransform& t)
	//- {
	//-		return t.m_rotation > 45;
	//- }
	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps, typename TCallable>
	auto cquery_manager::query_one(TCallable callback) -> flecs::entity
	{
		//- Use ad-hoc filter, as this does not require building tables
		//- and thus can be used inside a progress tick
		return m_world.query_builder<TComps...>()
			.build()
			.find(callback);
	}

	//- Note: use this very cautiously, querying all entities in a world can be very very expensive. For example, all entities
	//- have an identifier component, and you want to find entities that contain a substring in their name, then you would literally
	//- iterate the whole world with an expected hit to framerate
	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps, typename TCallable>
	auto cquery_manager::query_all(TCallable callback) -> entities_t
	{
		entities_t output;
		output.reserve(m_all_entities.size());

		m_world.each<TComps...>([&](flecs::entity e, TComps... args)
			{
				if (callback(std::forward<TComps>(args)...))
				{
					output.emplace_back(e);
				}
			});
		return output;
	}

} //- kokoro