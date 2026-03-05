#pragma once
#include <engine/world/component.hpp>
#include <engine/world/world.hpp>
#include <bgfx.h>

namespace kokoro::world::component
{
	//------------------------------------------------------------------------------------------------------------------------
	struct sviewport final : public kokoro::ecs::icomponent
	{
		DECLARE_COMPONENT(sviewport);

		bgfx::FrameBufferHandle m_framebuffer;
		bgfx::ViewId m_view;

		RTTR_ENABLE(kokoro::ecs::icomponent);
	};

} //- kokoro::world::component