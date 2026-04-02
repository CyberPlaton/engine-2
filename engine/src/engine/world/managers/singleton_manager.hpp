#pragma once
#include <flecs.h>
#include <rttr.h>
#include <set>
#include <string>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class csingleton_manager final
	{
	public:
		using component_types_t = std::set<std::string>;

		csingleton_manager() = default;
		~csingleton_manager() = default;

		void	world(flecs::world* w) { m_world = w; };
		void	init();
		void	shutdown();

		auto	registered() -> component_types_t;						//- Retrieve all singleton components registered for a world
		auto	all() -> component_types_t;								//- Retrieve all singletons of a world
		void	add(const rttr::variant& c);							//- Add a singleton component to the world
		template<typename TSingleton>
		void	add() { rttr::variant c = TSingleton{}; add(w, c); }
		void	remove(const rttr::type& t);							//- Remove a singleton component from the world
		template<typename TSingleton>
		void	remove() { remove(w, rttr::type::get<TSingleton>()); }

	private:
		flecs::world* m_world = nullptr;
		component_types_t m_all_singletons;		//- All registered singleton types for world
		component_types_t m_active_singletons;	//- Singletons of the world that are currently active, meaning they can be accessed and altered
	};

} //- kokoro