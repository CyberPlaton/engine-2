#include <entry.hpp>
#include <engine.hpp>

namespace kokoro::entry
{
	//------------------------------------------------------------------------------------------------------------------------
	int main(int argc, char* argv[])
	{
		auto& e = instance();
		if (!e.init())
		{
			return -1;
		}

		e.run();
		e.shutdown();
		return 0;
	}

} //- kokoro::entry