#include <entry.hpp>
#if PLATFORM_WINDOWS
#include <Windows.h>
#endif
#include <engine.hpp>
#include <sample.hpp>

//------------------------------------------------------------------------------------------------------------------------
static void configure(kokoro::cengine& engine)
{
	engine.new_layer<cgame>();
}

MAIN(configure);