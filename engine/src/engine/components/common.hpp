#pragma once
#include <engine/world/component.hpp>
#include <engine/world/world.hpp>

namespace kokoro::world::component
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

} //- kokoro::world::component

namespace kokoro::world::tag
{
	//- Tag an entity as invisible, i.e. it will not be drawn if it has a sprite component.
	//------------------------------------------------------------------------------------------------------------------------
	DECLARE_TAG(sinvisible);

	//- Tag indicating that an entity was created. Used to intercept entity creation/deletion events.
	//------------------------------------------------------------------------------------------------------------------------
	DECLARE_TAG(sentity);

} //- kokoro::world::tag