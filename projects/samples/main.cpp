#include <flecs.h>
#include <nlohmann.h>
#include <rttr.h>
#include <glfw.h>
#include <bx.h>
#include <bimg.h>
#include <bgfx.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct swindow final
{
	GLFWwindow* m_window			= nullptr;
	bgfx::FrameBufferHandle m_fbh	= BGFX_INVALID_HANDLE;
	uint16_t m_width				= 0;
	uint16_t m_height				= 0;
};

#define WINDOW_COUNT_MAX 8
swindow windows[WINDOW_COUNT_MAX];

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	if (!glfwInit())
	{
		return -1;
	}

	//- Init
	for (auto i = 0; i < WINDOW_COUNT_MAX; i++)
	{
		char str[256];
		bx::snprintf(str, BX_COUNTOF(str), "Window - %d", i);
		windows[i].m_window = glfwCreateWindow(640, 480, str, nullptr, nullptr);
		windows[i].m_width = 640;
		windows[i].m_height = 480;
	}

	//- Render init
	{
		const uint64_t reset = 0
			| BGFX_RESET_MSAA_X4
			| BGFX_RESET_MAXANISOTROPY
			| BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.vendorId = BGFX_PCI_ID_NVIDIA;
		init.resolution.width = windows[0].m_width;
		init.resolution.height = windows[0].m_height;
		init.resolution.reset = reset;
		init.type = bgfx::RendererType::Direct3D11;
		init.platformData.nwh = windows[0].m_window;
		init.platformData.ndt = nullptr;

		if (!bgfx::init(init))
		{
			return -1;
		}
	}


	//- Main Loop
	while (!glfwWindowShouldClose(windows[0].m_window))
	{
		glfwPollEvents();

		//- Drawing done for each of the created windows
		for (auto i = 0; i < WINDOW_COUNT_MAX; i++)
		{
			auto& window = windows[i];

			//- Check if we need to resize and recreate the windows framebuffer
			int width = 0, height = 0;
			glfwGetWindowSize(window.m_window, &width, &height);

			if (window.m_width != width ||
				window.m_height != height)
			{
				if (bgfx::isValid(window.m_fbh))
				{
					bgfx::destroy(window.m_fbh);
					window.m_fbh = BGFX_INVALID_HANDLE;
				}
				bgfx::frame();
				window.m_fbh = bgfx::createFrameBuffer(window.m_window, window.m_width, window.m_height);
			}
		}

		bgfx::touch(0);

		for (auto i = 0; i < WINDOW_COUNT_MAX; i++)
		{
			const bgfx::ViewId view = static_cast<uint16_t>(i);
			auto& window = windows[i];

			bgfx::setViewFrameBuffer(view, window.m_fbh);
			bgfx::setViewRect(view, 0, 0, window.m_width, window.m_height);
			bgfx::setViewClear(view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		}

		bgfx::frame();
	}

	//- Exit
	bgfx::shutdown();
	for (auto i = 0; i < WINDOW_COUNT_MAX; i++)
	{
		auto& window = windows[i];
		if (bgfx::isValid(window.m_fbh))
		{
			bgfx::destroy(window.m_fbh);
			window.m_fbh = BGFX_INVALID_HANDLE;
		}
		glfwDestroyWindow(windows[i].m_window);
	}
	return 0;
}