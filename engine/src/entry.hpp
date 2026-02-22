#pragma once

#define _KOKORO_MAIN_ENTRY_(argc, argv) \
kokoro::entry::main(argc, argv)

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) \
{ \
	return _KOKORO_MAIN_ENTRY_(0, nullptr); \
}
#else
int main(int argc, char* argv[]) \
{ \
	return _KOKORO_MAIN_ENTRY_(argc, argv); \
}
#endif

namespace kokoro::entry
{
	int main(int argc, char* argv[]);

} //- kokoro::entry
