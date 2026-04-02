#pragma once
#include <flecs.h>
#include <rttr.h>
#include <vector>
#include <set>
#include <unordered_map>
#include <string>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class centity_manager final
	{
	public:
		using entities_t = std::vector<flecs::entity>;
		using component_types_t = std::set<std::string>;
		using active_component_count_t = std::unordered_map<std::string, uint64_t>;
		using entity_component_types_t = std::unordered_map<uint64_t, component_types_t>;

		centity_manager() = default;
		~centity_manager() = default;

		void	world(flecs::world* w) { m_world = w; };
		void	init();
		void	shutdown();

		void	add(flecs::entity e, const rttr::variant& c);
		template<typename TComponent>
		void	add(flecs::entity e) { rttr::variant c = TComponent{}; add(e, c); }
		void	remove(flecs::entity e, const rttr::type& t);
		template<typename TComponent>
		void	remove(flecs::entity e) { remove(e, rttr::type::get<TComponent>()); }
		auto	create(std::string_view name, std::string_view uuid = {}, std::string_view parent = {}, bool prefab = false) -> flecs::entity;
		auto	find(std::string_view name_or_uuid) const -> flecs::entity;
		void	kill(flecs::entity e);
		void	kill(std::string_view name_or_uuid);
		void	kill(const entities_t& entities);
		template<typename TCallable>
		void	kill_if(TCallable callback);
		auto	comps(flecs::entity e) const -> component_types_t;
		auto	all() const -> entities_t { return m_all_entities; }

	private:
		flecs::world* m_world = nullptr;
		flecs::observer m_comps_observer;			//- Observer for added or removed components from entities
		entities_t m_all_entities;					//- All created entities for world
		component_types_t m_all_comps;				//- All registered component types for world
		entity_component_types_t m_entity_comps;	//- Mapping entities to their component types
		active_component_count_t m_active_comps;	//- Mapping component types and their count for a world allowing reasoning about memory usage
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TCallable>
	void centity_manager::kill_if(TCallable callback)
	{
		sworld::entities_t deathrow;
		deathrow.reserve(m_all_entities.size());

		for (const auto& e : m_all_entities)
		{
			if (callback(e))
			{
				deathrow.emplace_back(e);
			}
		}

		kill(deathrow);
	}

} //- kokoro