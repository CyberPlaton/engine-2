#pragma once
#include <core/uuid.hpp>
#include <engine/world/system.hpp>
#include <engine/world/module.hpp>
#include <engine/world/singleton.hpp>
#include <engine/world/component.hpp>
#include <engine/world/change_tracker.hpp>
#include <engine/scene.hpp>
#include <engine/math/aabb.hpp>
#include <engine/math/ray.hpp>
#include <box2d.h>

namespace kokoro
{
	namespace world
	{
		using entity_proxy_t = int;
		using query_t = uint64_t;

		namespace detail
		{
			//- TODO: these 3 functions should be moved somewhere else or refactored somehow. Used for change trackers, but really
			//- a bad place here.
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

			std::pair<bool, uint64_t> do_check_is_flecs_built_in_phase(std::string_view name);

		} //- detail

		//- TODO: move to own file.
		//- Representing a query into the world.
		//------------------------------------------------------------------------------------------------------------------------
		struct squery final
		{
			using data_t = rttr::variant;					//- Depending on intersection holding either a ray or an aabb
			using filters_t = std::vector<rttr::variant>;	//- Array of sfilter variants
			using result_t = rttr::variant;					//- Depending on the query type holding:
															//- type_any - bool
															//- type_all - std::vector< flecs::entity >
															//- type_count - uint64_t

			enum type : uint8_t
			{
				type_none = 0,
				type_any,		//- Return whether we encountered any entity
				type_all,		//- Return all encountered entities
				type_count,		//- Return the number of encountered entities
			};

			enum intersection : uint8_t
			{
				intersection_none = 0,
				intersection_ray,		//- Querying by raycast
				intersection_aabb,		//- Querying by aabb
			};

			filters_t m_filters;
			data_t m_data;
			type m_type					= type_none;
			intersection m_intersection	= intersection_none;
		};

		//- An entity representation used in the internally managed quad tree. Used by world and proxy manager with box2d.
		//------------------------------------------------------------------------------------------------------------------------
		struct sproxy final
		{
			math::aabb_t m_aabb;
			flecs::entity m_entity;
			entity_proxy_t m_id		= std::numeric_limits<entity_proxy_t>().max();
			unsigned m_query_key	= 0;
		};

		using world_flags_t = int;

		//------------------------------------------------------------------------------------------------------------------------
		enum world_flag : uint8_t
		{
			world_flag_none			= 0,
			world_flag_foreground	= 1 << 0,	//- Active world that renders to screen and uses GPU resources
			world_flag_background	= 1 << 1,	//- Active world that does not render to screen and does not use GPU resources
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

		//------------------------------------------------------------------------------------------------------------------------
		struct sworld final
		{
			static inline constexpr auto C_MASTER_QUERY_KEY_MAX	= 128;
			static inline constexpr auto C_INVALID_QUERY_ID		= std::numeric_limits<query_t>().max();

			using entities_t				= std::vector<flecs::entity>;
			using systems_t					= std::unordered_map<std::string, flecs::system>;
			using component_types_t			= std::set<std::string>;
			using active_component_count_t	= std::unordered_map<std::string, uint64_t>;
			using entity_component_types_t	= std::unordered_map<uint64_t, component_types_t>;
			using results_t					= std::unordered_map<query_t, squery::result_t>;
			using queries_t					= std::map<query_t, squery>;
			using change_trackers_t			= std::unordered_map<uint64_t, std::shared_ptr<ichange_tracker>>;

			struct sproxy_manager
			{
				using proxy_map_t			= std::unordered_map<core::cuuid, sproxy>;
				using user_data_map_t		= std::unordered_map<int, core::cuuid>;

				void	tick();
				void	update_proxy(flecs::entity e);
				void	destroy_proxy(flecs::entity e);
				void	create_proxy(flecs::entity e);
				void	queue_create_proxy(flecs::entity e);
				bool	has_proxy(flecs::entity e) const;
				sproxy&	proxy(int proxy_id);
				auto	tree() -> box2d::b2DynamicTree& { return m_quad_tree; }
				int		user_data_id(const flecs::entity& e) const;

				proxy_map_t m_proxies;
				user_data_map_t m_user_data;
				box2d::b2DynamicTree m_quad_tree;
				std::vector<flecs::entity> m_creation_queue;
			};

			sworld(std::string_view name);
			sworld(std::string_view name, sconfig cfg);
			~sworld();

			sworld& operator=(const sworld&) = delete;
			sworld(const sworld&) = delete;
			sworld(sworld&&) = delete;
			sworld& operator=(const sworld&&) = delete;

			const char*			name() const { return m_name.c_str(); }
			uint64_t			id() const { return m_id; }
			const sconfig&		config() const { return m_cfg; }
			bool				foreground() const;
			void				flags(world_flags_t value);

			flecs::world m_world;
			sproxy_manager m_proxy_manager;			//- Encapsulating proxy functionality. Responsible for keeping the game world and quad tree in sync
			change_trackers_t m_change_trackers;	//- All component archetype change trackers created for world
			systems_t m_all_systems;				//- All created systems for world
			entities_t m_all_entities;				//- All created entities for world
			component_types_t m_all_comps;			//- All registered component types for world
			component_types_t m_all_singletons;		//- All registered singleton types for world
			entity_component_types_t m_entity_comps;//- Mapping entities to their component types
			active_component_count_t m_active_comps;//- Mapping component types and their count for a world allowing reasoning about memory usage
			component_types_t m_active_singletons;	//- Singletons of the world that are currently active, meaning they can be accessed and altered

			results_t m_query_results;				//- Containing results of queries
			squery m_current_query;					//- Currently executed query
			queries_t m_queries;					//- All queries to be executed in order of their id, this way we maintain order and fast lookup
			query_t m_current_query_key = 0;		//- Id of the current query
			query_t m_next_query_key = 0;			//- Id to be assigned to the next created query

			flecs::observer m_proxy_observer;		//- Observer for creation of archetypes to create proxies
			flecs::observer m_comps_observer;		//- Observer for added or removed components from entities
			change_tracker_t m_transform_tracker;	//- Change tracker for any transform changes for updating proxies

		private:
			sconfig m_cfg;												//- World configuration. A dynamic object that can change throughout the lifetime of the world if necessery
			std::string m_name;											//- Given name of the world. Will be the same throughout the lifetime of the world
			uint64_t m_id = std::numeric_limits<uint64_t>().max();		//- Unique identifier of the world. Basically a hash value
		};

		namespace components
		{
			//- Minimal requirement component. Each entity or prefab has an identifier, beside their tag on what they are.
			//------------------------------------------------------------------------------------------------------------------------
			struct sidentifier final : public kokoro::ecs::icomponent
			{
				DECLARE_COMPONENT(sidentifier);

				std::string m_name;
				core::cuuid m_uuid;

				RTTR_ENABLE(kokoro::ecs::icomponent);
			};

			//- Parent and children strings are either names or uuids, basically anything using which an entity can reliably be
			//- looked up at runtime.
			//------------------------------------------------------------------------------------------------------------------------
			struct shierarchy final : public kokoro::ecs::icomponent
			{
				DECLARE_COMPONENT(shierarchy);

				core::cuuid m_parent = core::cuuid::C_INVALID_UUID;
				std::vector<core::cuuid> m_children;

				RTTR_ENABLE(kokoro::ecs::icomponent);
			};

			//- Each prefab instance has this component, it specifies from which resource it was instantiated. This is necessary as
			//- a tagging mechanism and for synchronizing changes in resources and instances in the world.
			//------------------------------------------------------------------------------------------------------------------------
			struct sprefab final : public kokoro::ecs::icomponent
			{
				DECLARE_COMPONENT(sprefab);

				std::string m_filepath;

				RTTR_ENABLE(kokoro::ecs::icomponent);
			};

			namespace tag
			{
				//- Tag an entity as invisible, i.e. it will not be drawn if it has a sprite component.
				//------------------------------------------------------------------------------------------------------------------------
				DECLARE_TAG(sinvisible);

				//- Tag indicating that an entity is a system.
				//------------------------------------------------------------------------------------------------------------------------
				DECLARE_TAG(ssystem);

				//- Tag indicating that an entity was created. Used to intercept entity creation/deletion events.
				//------------------------------------------------------------------------------------------------------------------------
				DECLARE_TAG(sentity);

			} //- tag

			sworld::component_types_t			registered(const sworld& w);							//- Retrieve all components registered for a world
			sworld::active_component_count_t	active(const sworld& w);								//- Retrieve currently active components for a world
			sworld::component_types_t			all(const sworld& w, flecs::entity e);					//- Retrieve all components of an entity

		} //- components

		namespace plugins
		{
			void								import(sworld& w, std::vector<std::string> plugins);			//- Import plugins of given type for world

		} //- plugins

		namespace modules
		{
			void								import(sworld& w, std::vector<std::string> modules);			//- Import modules for a world
			template<typename TModule>
			void								import(sworld& w, const kokoro::modules::sconfig& cfg)
			{
				//- Import module dependencies
				for (const auto& m : cfg.m_modules)
				{
					if (const auto t = rttr::type::get_by_name(m.data()); t.is_valid())
					{
						//- Calling module constructor that does the actual importing of the module
						t.create({ w });
					}
					else
					{
						log_error(fmt::format("Failed loading dependency module '{}' for world '{}' and module '{}', as the module is not reflected to RTTR!",
							m.data(), w.name(), cfg.m_name.data()));
					}

					w.m_world.module<TModule>(cfg.m_name.data());

					//- Register module components
					for (const auto& c : cfg.m_components)
					{
						if (const auto t = rttr::type::get_by_name(c.data()); t.is_valid())
						{
							//- Calling special component constructor that register the component to provided world
							t.create({ w.m_world });
						}
						else
						{
							log_error(fmt::format("Failed creating component '{}' for world '{}' and module '{}', as the component is not reflected to RTTR!",
								c.data(), w.name(), cfg.m_name.data()));
						}
					}

					//- Register module systems
					for (const auto& s : cfg.m_systems)
					{
						if (const auto t = rttr::type::get_by_name(s.data()); t.is_valid())
						{
							//- Calling system constructor that does the actual registration of the system
							t.create({ w });
						}
						else
						{
							log_error(fmt::format("Failed creating system '{}' for world '{}' and module '{}', as the system is not reflected to RTTR!",
								s.data(), w.name(), cfg.m_name.data()));
						}
					}
				}
			};

			template<typename... TComps>
			void								create_system(sworld& w, const kokoro::system::sconfig& cfg,	//- Create a system for world with given configuration and callback
													kokoro::system::system_callback_t<TComps...> callback)
			{
				CORE_ASSERT(!(algorithm::bit_check(cfg.m_flags, system::system_flag_multithreaded) &&
					algorithm::bit_check(cfg.m_flags, system::system_flag_immediate)), "A system cannot be multithreaded and immediate at the same time!");

				auto builder = w.m_world.system<TComps...>(cfg.m_name.c_str());

				//- Set options that are required before system entity creation
				{
					if (algorithm::bit_check(cfg.m_flags, system::system_flag_multithreaded))
					{
						builder.multi_threaded();
					}
					else if (algorithm::bit_check(cfg.m_flags, system::system_flag_immediate))
					{
						builder.immediate();
					}
				}

				if (const auto duplicate_system = find_system(w, cfg.m_name); duplicate_system)
				{
					log_warn(fmt::format("Trying to create a system with same name twice '{}'. Second definition ignored!", cfg.m_name));
					return;
				}

				if (cfg.m_interval != 0)
				{
					builder.interval(cfg.m_interval);
				}

				auto system = builder.each(callback);

				system.template add<components::tag::ssystem>();

				//- Set options that are required after system entity creation
				{
					for (const auto& after : cfg.m_run_after)
					{
						if (const auto [result, phase] = detail::do_check_is_flecs_built_in_phase(after); result)
						{
							system.add(flecs::Phase).depends_on(phase);
						}
						else
						{
							if (auto e = find_system(w, after.c_str()); e)
							{
								system.add(flecs::Phase).depends_on(e);
							}
							else
							{
								log_error(fmt::format("Dependency (run after) system '{}' for system '{}' could not be found!",
									after, cfg.m_name));
							}
						}
					}

					for (const auto& before : cfg.m_run_before)
					{
						if (const auto [result, phase] = detail::do_check_is_flecs_built_in_phase(before); result)
						{
							system.add(flecs::Phase).depends_on(phase);
						}
						else
						{
							if (auto e = find_system(w, before.c_str()); e)
							{
								e.add(flecs::Phase).depends_on(system);
							}
							else
							{
								log_error(fmt::format("Dependent (run before) system '{}' for system '{}' could not be found!",
									before, cfg.m_name));
							}
						}
					}
				}
				w.m_all_systems[cfg.m_name] = system;
			}
			void								create_task(sworld& w, const kokoro::system::sconfig& cfg,		//- Create a task for world with given configuration and callback
													kokoro::system::task_callback_t callback);
			flecs::system						find_system(const sworld& w, std::string_view name);			//- Find a system for a world by its name

		} //- modules

		namespace singletons
		{
			sworld::component_types_t			registered(const sworld& w);							//- Retrieve all singleton components registered for a world
			sworld::component_types_t			all(const sworld& w);									//- Retrieve all singletons of a world
			void								add(sworld& w, const rttr::variant& c);					//- Add a singleton component to the world
			template<typename TSingleton> void	add(sworld& w) { rttr::variant c = TSingleton{}; add(w, c); }
			void								remove(sworld& w, const rttr::type& t);					//- Remove a singleton component from the world
			template<typename TSingleton> void	remove(sworld& w) { remove(w, rttr::type::get<TSingleton>()); }

		} //- singletons

		namespace entity
		{
			void								add(sworld& w, flecs::entity e, const rttr::variant& c);//- Add a component to an entity
			template<typename TComponent> void	add(sworld& w, flecs::entity e) { rttr::variant c = TComponent{}; add(w, e, c); }
			void								remove(sworld& w, flecs::entity e, const rttr::type& t);//- Remove a component from an entity
			template<typename TComponent> void	remove(sworld& w, flecs::entity e) { remove(w, e, rttr::type::get<TComponent>()); }
			flecs::entity						create(sworld& w, std::string_view name,				//- Create a new entity for the world
													std::string_view uuid = {},
													std::string_view parent = {},
													bool is_prefab = false);
			flecs::entity						find(const sworld& w, std::string_view name_or_uuid);	//- Find an entity by its name or the uuid
			void								kill(sworld& w, flecs::entity e);						//- Kill an entity
			void								kill(sworld& w, std::string_view name_or_uuid);			//- Kill an entity with given name or uuid recursively
			void								kill(sworld& w, const sworld::entities_t& entities);	//- Kill all entities provided
			template<typename TCallable>
			void								kill_if(sworld& w, TCallable callback)					//- Kill all entities that are satisfying the callback function
			{
				sworld::entities_t deathrow;
				deathrow.reserve(w.m_all_entities.size());

				for (const auto& e : w.m_all_entities)
				{
					if (callback(e))
					{
						deathrow.emplace_back(e);
					}
				}

				kill(w, deathrow);
			};

		} //- entity

		namespace query
		{
			query_t								create(sworld& w, squery::type type,					//- Create a query into the world. Use the returned id to check if query is ready and has results
													squery::intersection intersection,
													squery::data_t data,
													squery::filters_t filters = {});
			squery::result_t					immediate(sworld& w, squery::type type,					//- Create a query into the world that executes immediately and blocking
													squery::intersection intersection,
													squery::data_t data,
													squery::filters_t filters = {});
			std::optional<squery::result_t>		result(sworld& w, query_t id);							//- Try to get the result of a query

			//- Create a component change tracker for a world. The tracker triggers once archetypes were modified
			//------------------------------------------------------------------------------------------------------------------------
			template<typename... TComps>
			change_tracker_t					change_tracker(sworld& w)
			{
				const auto h = kokoro::world::detail::comps_hash<TComps...>();

				if (const auto it = w.m_change_trackers.find(h); it == w.m_change_trackers.end())
				{
					auto [_, success] = w.m_change_trackers.emplace(std::piecewise_construct,
						std::forward_as_tuple(h),
						std::forward_as_tuple(std::move(std::make_shared<cchange_tracker<TComps...>>(w.m_world))));

					if (!success)
					{
						return {};
					}
				}

				return w.m_change_trackers[h];
			}

			//- Ex.:
			//- query::one<ecs::stransform>([](const ecs::stransform& t)
			//- {
			//-		return t.m_rotation > 45;
			//- }
			//------------------------------------------------------------------------------------------------------------------------
			template<typename... TComps, typename TCallable>
			flecs::entity						one(const sworld& w, TCallable callback)				//- Query for one entity in world meeting some conditions
			{
				//- Use ad-hoc filter, as this does not require building tables
				//- and thus can be used inside a progress tick
				return w.m_world.query_builder<TComps...>()
					.build()
					.find(callback);
			};

			//- Note: use this very cautiously, querying all entities in a world can be very very expensive. For example, all entities
			//- have an identifier component, and you want to find entities that contain a substring in their name, then you would literally
			//- iterate the whole world with an expected hit to framerate
			//------------------------------------------------------------------------------------------------------------------------
			template<typename... TComps, typename TCallable>
			sworld::entities_t					all(const sworld& w, TCallable callback)				//- Query for all entities in world meeting some conditions
			{
				sworld::entities_t output;
				output.reserve(w.m_all_entities.size());

				w.m_world.each<TComps...>([&](flecs::entity e, TComps... args)
					{
						if (callback(std::forward<TComps>(args)...))
						{
							output.emplace_back(e);
						}
					});
				return output;
			};

			namespace detail
			{
				bool							query_callback(int proxy_id, int user_data, void* ctx);	//- Internal callback for box2d for query execution by AABB
				float							raycast_callback(const box2d::b2RayCastInput* input,	//- Internal callback for box2d for query execution by raycast
													int proxy_id, int user_data, void* ctx);

			} //- detail

		} //- query

		sscene						as_scene(const sworld& w);												//- Retrieve the scene representation for the given world
		math::aabb_t				visible_area(const sworld& w,											//- Calculate the visible are for the worlds main camera
			float width_scale = 1.0f,
			float height_scale = 1.0f);
		std::vector<flecs::entity>	visible_entities(const sworld& w, const math::aabb_t& area);			//- Retrieve all visible entities in a world for a given area
		sworld::entities_t			entities(const sworld& w);												//- Retrieve all entities in a world
		void						tick(sworld& w, float dt);
		sworld*						deserialize(const nlohmann::json& json);
		nlohmann::json				serialize(const sworld& w);
		std::string					filepath(int id);
		sworld*						create(std::string_view name_or_filepath,								//- Create a new world that will be deserialized from file or freshly made if file was not found
										std::optional<sconfig> cfg = std::nullopt);
		sworld* active();																					//- Retrieve the world currently active in the foreground
		void						promote(sworld& w);														//- Set the given world as active in the foreground. This will automatically demote the previously active one
		void						demote(sworld& w);														//- Set the given world as background world
		void						destroy(std::string_view name_or_filepath);
		void						cleanup();
		sworld*						find(std::string_view name_or_filepath);								//- Retrieve a world by name or fielpath it was loaded with

	} //- world

} //- kokoro
