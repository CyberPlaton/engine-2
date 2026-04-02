#pragma once
#include <engine/iservice.hpp>
#include <engine/services/resource_manager_service.hpp>
#include <engine/world/world.hpp>
#include <string_view>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class cworld_service final : public iservice
	{
	public:
		cworld_service() = default;
		~cworld_service() = default;

		bool		init() override final;
		void		post_init() override final;
		void		shutdown() override final;
		void		update(float dt) override final;

		void		promote(cview<cworld> world);
		inline auto	active() const -> cview<cworld> { return m_active; }

	private:
		cview<cworld> m_active;
	};

} //- kokoro