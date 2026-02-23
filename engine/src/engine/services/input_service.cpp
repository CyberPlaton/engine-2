#include <engine/services/input_service.hpp>
#include <engine/services/window_service.hpp>
#include <engine.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::init()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cinput_service::post_init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cinput_service::shutdown()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cinput_service::update(float dt)
	{
		m_previous = std::move(m_current);
		m_current = instance().service<cwindow_service>().window_state();
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::key_pressed(int key) const
	{
		return m_previous.m_keys[key] == GLFW_RELEASE &&
			m_current.m_keys[key] == GLFW_PRESS;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::key_held(int key) const
	{
		return m_previous.m_keys[key] == GLFW_PRESS &&
			m_current.m_keys[key] == GLFW_PRESS;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::key_released(int key) const
	{
		return m_previous.m_keys[key] == GLFW_PRESS &&
			m_current.m_keys[key] == GLFW_RELEASE;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::button_pressed(int button) const
	{
		return m_previous.m_buttons[button] == GLFW_RELEASE &&
			m_current.m_buttons[button] == GLFW_PRESS;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::button_held(int button) const
	{
		return m_previous.m_buttons[button] == GLFW_PRESS &&
			m_current.m_buttons[button] == GLFW_PRESS;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::button_released(int button) const
	{
		return m_previous.m_buttons[button] == GLFW_PRESS &&
			m_current.m_buttons[button] == GLFW_RELEASE;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::gamepad_button_pressed(int button) const
	{
		return m_previous.m_gamepad[button] == GLFW_RELEASE &&
			m_current.m_gamepad[button] == GLFW_PRESS;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::gamepad_button_held(int button) const
	{
		return m_previous.m_gamepad[button] == GLFW_PRESS &&
			m_current.m_gamepad[button] == GLFW_PRESS;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cinput_service::gamepad_button_released(int button) const
	{
		return m_previous.m_gamepad[button] == GLFW_PRESS &&
			m_current.m_gamepad[button] == GLFW_RELEASE;
	}

} //- kokoro