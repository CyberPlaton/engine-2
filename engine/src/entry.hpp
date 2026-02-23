#pragma once

namespace kokoro::entry
{
	int main(int argc, char* argv[]);

} //- kokoro::entry

#define KOKORO_MAIN_ENTRY(argc, argv) \
kokoro::entry::main(argc, argv)

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	return KOKORO_MAIN_ENTRY(0, nullptr);
}
#else
int main(int argc, char* argv[])
{
	return KOKORO_MAIN_ENTRY(argc, argv);
}
#endif
