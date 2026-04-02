#include <engine/world/managers/entity_manager.hpp>
#include <engine/world/component.hpp>
#include <engine/components/common.hpp>
#include <engine/components/transforms.hpp>
#include <engine/services/log_service.hpp>
#include <engine.hpp>
#include <fmt.h>

namespace kokoro
{
	namespace
	{
		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		rttr::variant invoke_static_function(rttr::type class_type, rttr::string_view function_name, ARGS&&... args)
		{
			if (const auto m = class_type.get_method(function_name); m.is_valid())
			{
				return m.invoke({}, args...);
			}
			return {};
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	void centity_manager::init()
	{
		m_comps_observer = m_world
			->observer<>()
			.event(flecs::OnAdd)
			.event(flecs::OnRemove)
			.with(flecs::Wildcard)
			.ctx(this)
			.each([](flecs::iter& it, uint64_t i)
				{
					if (auto* manager = reinterpret_cast<centity_manager*>(it.ctx()); manager)
					{
						auto entity = it.entity(i);
						std::string string = it.event_id().str().c_str();

						//- Tag components are required to be inside the 'tag' namespace,
						//- thus when we strip the name we must retain 'tag::' for those components,
						//- otherwise the engine will not recognize them as correct components.
						if (const auto last = string.find_last_of("."); last != std::string::npos)
						{
							const auto is_tag = string.find(".tag") != std::string::npos;
							auto c = string.substr(last + 1);
							if (is_tag)
							{
								c = fmt::format("tag::{}", c);
							}

							if (it.event() == flecs::OnAdd)
							{
								instance().service<clog_service>().debug("Component added '{}({})' to entity '{}'",
									c, string, entity.id());

								//- name of the component will be in form 'ecs.transform', we have to format the name first
								manager->m_entity_comps[entity].insert(c);
							}
							else if (it.event() == flecs::OnRemove)
							{
								instance().service<clog_service>().debug("Component removed '{}({})' from entity '{}'",
									c, string, entity.id());

								manager->m_entity_comps[entity].erase(c);
							}
						}
					}
				});
	}

	//------------------------------------------------------------------------------------------------------------------------
	void centity_manager::shutdown()
	{
		m_comps_observer.destruct();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void centity_manager::add(flecs::entity e, const rttr::variant& c)
	{
		invoke_static_function(c.get_type(), kokoro::ecs::C_COMPONENT_SET_FUNC_NAME, e, c);
		m_entity_comps[e.id()].insert(c.get_type().get_name().data());
	}

	//------------------------------------------------------------------------------------------------------------------------
	void centity_manager::remove(flecs::entity e, const rttr::type& t)
	{
		invoke_static_function(t, kokoro::ecs::C_COMPONENT_REMOVE_FUNC_NAME, e);

		std::string_view type_name = t.get_name().data();
		auto& comps = m_entity_comps[e.id()];

		for (const auto& ct : comps)
		{
			if (ct == type_name)
			{
				comps.erase(ct);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto centity_manager::create(std::string_view name, std::string_view uuid /*= {}*/, std::string_view parent /*= {}*/,
		bool prefab /*= false*/) -> flecs::entity
	{
		flecs::entity out = m_world->entity(name.data());

		if (!prefab)
		{
			add<world::tag::sentity>(out);
		}
		else
		{
			add<world::component::sprefab>(out);
		}

		add<world::component::sidentifier>(out);
		add<world::component::shierarchy>(out);
		add<world::component::sworld_transform>(out);
		add<world::component::slocal_transform>(out);

		//- Identifier component holding the unique entity uuid and a name
		auto* id = out.get_mut<world::component::sidentifier>();
		id->m_name = name.data();
		id->m_uuid = uuid.empty() ? core::cuuid().string() : std::string(uuid.data());
		out.set_alias(id->m_uuid.string().data());

		if (!parent.empty())
		{
			if (auto e_parent = find(parent); e_parent.is_valid())
			{
				const auto* p_identifier = e_parent.get<world::component::sidentifier>();
				auto* p_hierarchy = e_parent.get_mut<world::component::shierarchy>();

				auto* hierarchy = out.get_mut<world::component::shierarchy>();
				hierarchy->m_parent = p_identifier->m_uuid;
				p_hierarchy->m_children.emplace_back(id->m_uuid);
				out.child_of(e_parent);
			}
		}

		//- Track created entity and create a proxy too
		m_all_entities.emplace_back(out);
		/*m_proxy_manager.create_proxy(out);*/

		return out;
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto centity_manager::find(std::string_view name_or_uuid) const -> flecs::entity
	{
		return m_world->lookup(name_or_uuid.data());
	}

	//------------------------------------------------------------------------------------------------------------------------
	void centity_manager::kill(flecs::entity e)
	{
		const auto* id = e.get<world::component::sidentifier>();
		const auto* hierarchy = e.get<world::component::shierarchy>();

		//- Copy array of children, as we wil erase entries from hierarchy children array recursively
		auto children = hierarchy->m_children;
		for (const auto& kid : children)
		{
			kill(kid.string());
		}

		//- Erase kid from parents children array if it has one
		if (hierarchy->m_parent != core::cuuid::C_INVALID_UUID)
		{
			if (auto e_parent = find(hierarchy->m_parent.string()); e_parent.is_valid())
			{
				auto* hierarchy = e_parent.get_mut<world::component::shierarchy>();
				const auto s = id->m_uuid;
				const auto h = s.hash();

				for (auto i = 0; i < hierarchy->m_children.size(); ++i)
				{
					const auto& k = hierarchy->m_children[i];

					if (k.hash() == h)
					{
						hierarchy->m_children.erase(hierarchy->m_children.begin() + i);
						break;
					}
				}
			}
		}

		//- Destroy entity within ecs world to free components and data
		e.destruct();

		//- Untrack entity and destroy its proxy
		for (auto i = 0; i < m_all_entities.size(); ++i)
		{
			const auto& entity = m_all_entities[i];

			if (entity == e)
			{
				m_all_entities.erase(m_all_entities.begin() + i);
				break;
			}
		}

		/*m_proxy_manager.destroy_proxy(e);*/
	}

	//------------------------------------------------------------------------------------------------------------------------
	void centity_manager::kill(std::string_view name_or_uuid)
	{
		if (auto e = find(name_or_uuid); e.is_valid())
		{
			kill(e);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void centity_manager::kill(const entities_t& entities)
	{
		for (const auto& e : entities)
		{
			kill(e);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto centity_manager::comps(flecs::entity e) const -> component_types_t
	{
		if (const auto it = m_entity_comps.find(e.id()); it != m_entity_comps.end())
		{
			return it->second;
		}
		return {};
	}

} //- kokoro