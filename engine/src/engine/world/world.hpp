#pragma once
#include <engine/world/managers/entity_manager.hpp>
#include <engine/world/managers/query_manager.hpp>
#include <engine/world/managers/singleton_manager.hpp>
#include <engine/world/managers/system_manager.hpp>
#include <engine/world/change_tracker.hpp>
#include <engine/world/module.hpp>
#include <engine/scene.hpp>
#include <rttr.h>
#include <nlohmann.h>
#include <string_view>
#include <optional>
#include <memory>

namespace kokoro
{
	namespace world
	{
		using world_flags_t = int;

		enum world_flag : uint8_t
		{
			world_flag_none = 0,
			world_flag_foreground = 1 << 0,	//- Active world that renders to screen and uses GPU resources
			world_flag_background = 1 << 1,	//- Active world that does not render to screen and does not use GPU resources
		};

		//- Configuration of the world to be created. Note that plugins and modules do not need to be set, but are rather
		//- automatically provided from the engine.
		//------------------------------------------------------------------------------------------------------------------------
		struct sconfig final
		{
			std::vector<std::string> m_plugins;
			std::vector<std::string> m_modules;
			unsigned m_threads = 1;
			world_flags_t m_flags = 0;

			RTTR_ENABLE();
		};

	} //- world

	//- TODO: missing the serialized singletons here
	//------------------------------------------------------------------------------------------------------------------------
	struct sworld_snapshot final
	{
		world::sconfig m_cfg;
		std::string m_name;
		sscene m_scene;
		RTTR_ENABLE();
	};

	//------------------------------------------------------------------------------------------------------------------------
	class cworld
	{
	public:
		using entities_t = std::vector<flecs::entity>;

		static std::optional<cworld>	load(const rttr::variant& snapshot);
		static void						unload(cworld& world);

		cworld(std::string_view name);
		cworld(std::string_view name, world::sconfig cfg);
		cworld(cworld&&) = default;
		cworld& operator=(cworld&&) = default;
		cworld(const cworld&) = delete;
		cworld& operator=(const cworld&) = delete;
		~cworld();

		inline auto&	singleton_manager() { return m_managers->m_singleton_manager; }
		inline auto&	system_manager() { return m_managers->m_system_manager; }
		inline auto&	query_manager() { return m_managers->m_query_manager; }
		inline auto&	entity_manager() { return m_managers->m_entity_manager; }
		inline auto		singleton_manager() const -> const auto& { return m_managers->m_singleton_manager; }
		inline auto		system_manager() const -> const auto& { return m_managers->m_system_manager; }
		inline auto		query_manager() const -> const auto& { return m_managers->m_query_manager; }
		inline auto		entity_manager() const -> const auto& { return m_managers->m_entity_manager; }

		void			import_modules(std::vector<std::string> modules);

		template<typename TModule>
		void			import_module(const kokoro::modules::sconfig& cfg);

		//- Retrieve the scene representation for the given world
		sscene			as_scene();
		auto			as_snapshot() -> sworld_snapshot;
		void			from_snapshot(const sworld_snapshot& snaps);

		//- Calculate the visible are for the worlds main camera
		math::aabb_t	visible_area(float width_scale = 1.0f, float height_scale = 1.0f);

		//- Retrieve all visible entities in a world for a given area
		auto			visible_entities(const math::aabb_t& area) -> entities_t;

		void			tick(float dt);

		inline auto		w() -> flecs::world& { return m_world; }
		inline auto		w() const -> const flecs::world& { return m_world; }

	private:
		struct smanagers
		{
			csingleton_manager m_singleton_manager;
			csystem_manager m_system_manager;
			cquery_manager m_query_manager;
			centity_manager m_entity_manager;
		};

		std::unique_ptr<smanagers> m_managers;
		flecs::world m_world;
		world::sconfig m_cfg;
		std::string m_name;
		world::change_tracker_t m_transform_tracker;	//- Change tracker for any transform changes for updating proxies

	private:
		void world_to_scene_entity_rec(flecs::entity e, sscene::entities_t& entities);
		void scene_to_world_entity_rec(const sscene::sentity& e, const sscene::sentity* p = nullptr);
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TModule>
	void cworld::import_module(const kokoro::modules::sconfig& cfg)
	{
		//- Import module dependencies
		for (const auto& m : cfg.m_modules)
		{
			if (const auto t = rttr::type::get_by_name(m.data()); t.is_valid())
			{
				//- Calling module constructor that does the actual importing of the module
				t.create({ this });
			}

			m_world.module<TModule>(cfg.m_name.data());
		}

		//- Register module components
		for (const auto& c : cfg.m_components)
		{
			if (const auto t = rttr::type::get_by_name(c.data()); t.is_valid())
			{
				//- Calling special component constructor that register the component to provided world
				t.create({ m_world });
			}
		}

		//- Register module systems
		for (const auto& s : cfg.m_systems)
		{
			if (const auto t = rttr::type::get_by_name(s.data()); t.is_valid())
			{
				//- Calling system constructor that does the actual registration of the system
				t.create({ this });
			}
		}
	}

} //- kokoro