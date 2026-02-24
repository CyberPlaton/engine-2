#include <engine/window.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	void* native_window_handle(GLFWwindow* window)
	{
#if PLATFORM_WINDOWS
		return glfwGetWin32Window(window);
#elif PLATFORM_LINUX
		return (void*)(uintptr_t)glfwGetX11Window(window);
#elif PLATFORM_MACOSX
		return glfwGetCocoaWindow(window);
#else
		return nullptr;
#endif
	}

} //- kokoro