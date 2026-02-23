#pragma once
#include <engine/iservice.hpp>
#include <engine/window.hpp>
#include <utility>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class cwindow_service final : public iservice
	{
	public:
		cwindow_service() = default;
		~cwindow_service() = default;

		bool								init() override final;
		void								post_init() override final;
		void								shutdown() override final;
		void								update(float dt) override final;
		swindow								create(uint16_t w, uint16_t h);
		swindow								main_window() const;
		std::pair<uint16_t, uint16_t>		window_size() const;
		swindow_state						window_state() const;
	};

} //- kokoro