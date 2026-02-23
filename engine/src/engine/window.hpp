#pragma once
#include <glfw.h>
#include <bgfx.h>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	enum modifier_key : uint8_t
	{
		modifier_key_none		= 0,
		modifier_key_shift		= GLFW_MOD_SHIFT,
		modifier_key_control	= GLFW_MOD_CONTROL,
		modifier_key_alt		= GLFW_MOD_ALT,
		modifier_key_super		= GLFW_MOD_SUPER,
		modifier_key_caps_lock	= GLFW_MOD_CAPS_LOCK,
		modifier_key_num_lock	= GLFW_MOD_NUM_LOCK,
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct swindow_state final
	{
		bool m_keys[GLFW_KEY_LAST]					= { 0 };
		bool m_buttons[GLFW_MOUSE_BUTTON_LAST]		= { 0 };
		bool m_joystick[GLFW_JOYSTICK_LAST]			= { 0 };
		bool m_gamepad[GLFW_GAMEPAD_BUTTON_LAST]	= { 0 };
		float m_x									= 0.0f;
		float m_y									= 0.0f;
		float m_scroll								= 0.0f;
		int m_modifiers								= 0;
		char m_input_character						= 0;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct swindow final
	{
		GLFWwindow* m_window					= nullptr;
		bgfx::FrameBufferHandle m_framebuffer	= BGFX_INVALID_HANDLE;
		uint16_t m_width						= 0;
		uint16_t m_height						= 0;
	};

} //- kokoro