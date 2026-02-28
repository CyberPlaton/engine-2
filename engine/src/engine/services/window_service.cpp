#include <engine/services/window_service.hpp>
#include <engine.hpp>

namespace kokoro
{
	namespace
	{
		swindow window;
		swindow_state state;

		//------------------------------------------------------------------------------------------------------------------------
		void glfw_gamepad_connection_callback(int gamepad_id, int event)
		{

		}

		//------------------------------------------------------------------------------------------------------------------------
		void glfw_window_size_callback(GLFWwindow* /*window*/, int width, int height)
		{
			window.m_width = static_cast<uint16_t>(width);
			window.m_height = static_cast<uint16_t>(height);
		}

		//------------------------------------------------------------------------------------------------------------------------
		void glfw_key_callback(GLFWwindow* /*window*/, int key, int scancode, int action, int mods)
		{
			state.m_keys[key] = static_cast<bool>(action);
			state.m_modifiers = mods;
		}

		//------------------------------------------------------------------------------------------------------------------------
		void glfw_mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int mods)
		{
			state.m_buttons[button] = static_cast<bool>(action);
			state.m_modifiers = mods;
		}

		//------------------------------------------------------------------------------------------------------------------------
		void glfw_cursor_callback(GLFWwindow* /*window*/, double mx, double my)
		{
			state.m_x = static_cast<float>(mx);
			state.m_y = static_cast<float>(my);
		}

		//------------------------------------------------------------------------------------------------------------------------
		void glfw_scroll_callback(GLFWwindow* /*window*/, double dx, double dy)
		{
			state.m_scroll += static_cast<float>(dy);
		}

		//------------------------------------------------------------------------------------------------------------------------
		void glfw_character_callback(GLFWwindow* /*window*/, unsigned codepoint)
		{
			state.m_input_character = static_cast<char>(codepoint);
		}

		//------------------------------------------------------------------------------------------------------------------------
		void glfw_window_close_callback(GLFWwindow* /*window*/)
		{
			instance().exit();
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	bool cwindow_service::init()
	{
		if (!glfwInit())
		{
			return false;
		}
		/*glfwSetErrorCallback(glfw_error_callback);*/
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		create(1024, 960);
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cwindow_service::post_init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cwindow_service::shutdown()
	{
		glfwTerminate();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cwindow_service::update(float dt)
	{
		glfwPollEvents();
	}

	//------------------------------------------------------------------------------------------------------------------------
	swindow cwindow_service::create(uint16_t w, uint16_t h)
	{
		if (window.m_window)
		{
			return window;
		}

		window.m_window = glfwCreateWindow(static_cast<int>(w), static_cast<int>(h), "Main Window", nullptr, nullptr);
		window.m_width = w;
		window.m_height = h;

		glfwSetCursor(window.m_window, glfwCreateStandardCursor(GLFW_ARROW_CURSOR));
		glfwSetKeyCallback(window.m_window, glfw_key_callback);
		glfwSetMouseButtonCallback(window.m_window, glfw_mouse_button_callback);
		glfwSetCursorPosCallback(window.m_window, glfw_cursor_callback);
		glfwSetWindowSizeCallback(window.m_window, glfw_window_size_callback);
		glfwSetScrollCallback(window.m_window, glfw_scroll_callback);
		glfwSetCharCallback(window.m_window, glfw_character_callback);
		glfwSetWindowCloseCallback(window.m_window, glfw_window_close_callback);
		glfwSetJoystickCallback(glfw_gamepad_connection_callback);
		return window;
	}

	//------------------------------------------------------------------------------------------------------------------------
	swindow cwindow_service::main_window() const
	{
		return window;
	}

	//------------------------------------------------------------------------------------------------------------------------
	std::pair<uint16_t, uint16_t> cwindow_service::window_size() const
	{
		return { window.m_width, window.m_height };
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cwindow_service::window_state() const -> swindow_state
	{
		return state;
	}

} //- kokoro