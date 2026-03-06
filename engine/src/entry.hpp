#pragma once

namespace kokoro
{
	class cengine;
} //- kokoro

namespace kokoro::entry
{
	typedef void(*config_func_t)(cengine&);

	int main(config_func_t cfg, int argc, char* argv[]);

} //- kokoro::entry

#if PLATFORM_WINDOWS
	#define MAIN(cfg) \
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) \
	{ \
		return kokoro::entry::main(cfg, 0, nullptr); \
	}
#else
	#define MAIN(cfg) \
	int main(int argc, char* argv[]) \
	{ \
		return kokoro::entry::main(cfg, argc, argv); \
	}
#endif