#pragma once
#include <engine/world/component.hpp>
#include <engine/world/world.hpp>
#include <engine/postprocess/postprocess.hpp>
#include <engine/services/resource_manager_service.hpp>

namespace kokoro::world::component
{
	//------------------------------------------------------------------------------------------------------------------------
	struct spostprocess_volume final : public kokoro::ecs::icomponent
	{
		DECLARE_COMPONENT(spostprocess_volume);

		cview<spostprocess> m_postprocess;

		RTTR_ENABLE(kokoro::ecs::icomponent);
	};

} //- kokoro::world::component