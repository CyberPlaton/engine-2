#pragma once
#include <engine/ilayer.hpp>

//------------------------------------------------------------------------------------------------------------------------
class cgame final : public kokoro::ilayer
{
public:
	cgame() = default;
	~cgame() override = default;

	bool	init() override final;
	void	post_init() override final;
	void	shutdown() override final;
	void	update(float dt) override final;
};