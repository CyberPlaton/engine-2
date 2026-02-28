#pragma once

namespace kokoro::entry
{
	int main(int argc, char* argv[]);

} //- kokoro::entry

#if PLATFORM_WINDOWS
	#define MAIN \
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) \
	{ \
		return kokoro::entry::main(0, nullptr); \
	}
#else
	#define MAIN \
	int main(int argc, char* argv[]) \
	{ \
		return kokoro::entry::main(argc, argv); \
	}
#endif