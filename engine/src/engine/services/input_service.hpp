#pragma once
#include <engine/iservice.hpp>
#include <engine/window.hpp>
#include <utility>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class cinput_service final : public iservice
	{
	public:
		cinput_service() = default;
		~cinput_service() = default;

		bool		init() override final;
		void		post_init() override final;
		void		shutdown() override final;
		void		update(float dt) override final;

		bool		key_pressed(int key) const;
		bool		key_held(int key) const;
		bool		key_released(int key) const;

		bool		button_pressed(int button) const;
		bool		button_held(int button) const;
		bool		button_released(int button) const;

		bool		gamepad_button_pressed(int button) const;
		bool		gamepad_button_held(int button) const;
		bool		gamepad_button_released(int button) const;

		auto		mouse_position() const -> std::pair<float, float> { return { m_current.m_x, m_current.m_y }; }
		int			modifiers() const { return m_current.m_modifiers; }
		float		scroll() const { return m_current.m_scroll; }
		char		input_character() const { return m_current.m_input_character; }

	private:
		swindow_state m_current;
		swindow_state m_previous;
	};

} //- kokoro