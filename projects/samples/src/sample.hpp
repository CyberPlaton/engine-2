#pragma once
#include <engine/ilayer.hpp>
#include <engine/world/world.hpp>
#include <engine/render/debug.hpp>

//------------------------------------------------------------------------------------------------------------------------
class cgame final : public kokoro::ilayer
{
public:
	cgame() = default;

	bool	init() override final;
	void	post_init() override final;
	void	shutdown() override final;
	void	update(float dt) override final;

private:
	kokoro::cdebug_drawer m_dd;
	kokoro::cview<kokoro::cworld> m_world;
};