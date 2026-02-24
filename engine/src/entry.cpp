#include <entry.hpp>
#include <engine/services/input_service.hpp>
#include <engine/services/window_service.hpp>
#include <engine/services/render_service.hpp>
#include <engine.hpp>

namespace kokoro::entry
{
	//------------------------------------------------------------------------------------------------------------------------
	int main(int argc, char* argv[])
	{
		auto& e = instance();

		e.new_service<cwindow_service>()
			.new_service<cinput_service>()
			.new_service<crender_service>();

		if (!e.init())
		{
			return -1;
		}

		e.run();
		e.shutdown();
		return 0;
	}

} //- kokoro::entry