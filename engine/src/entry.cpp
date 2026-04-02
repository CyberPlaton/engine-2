#include <entry.hpp>
#include <engine/services/input_service.hpp>
#include <engine/services/window_service.hpp>
#include <engine/services/render_service.hpp>
#include <engine/services/thread_service.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/resource_manager_service.hpp>
#include <engine/services/log_service.hpp>
#include <engine/services/world_service.hpp>
#include <engine/render/mesh.hpp>
#include <engine/render/texture.hpp>
#include <engine/material/material.hpp>
#include <engine/effect/effect.hpp>
#include <engine.hpp>

namespace kokoro::entry
{
	//------------------------------------------------------------------------------------------------------------------------
	int main(config_func_t cfg, int argc, char* argv[])
	{
		auto& e = instance();

		//- Add our services
		e.new_service<clog_service>()
			.new_service<cwindow_service>()
			.new_service<cinput_service>()
			.new_service<cthread_service>()
			.new_service<cvirtual_filesystem_service>()
			.new_service<crender_service>()
			.new_service<cresource_manager_service>()
			.new_service<cworld_service>();

		e.service<clog_service>().set_level(clog_service::level_trace);

		auto& rms = e.service<cresource_manager_service>();
		rms.new_manager<stexture, stexture_snapshot, false>()
			.new_manager<seffect, seffect_snapshot, false>()
			.new_manager<smesh, smesh_snapshot>()
			.new_manager<smaterial, smaterial_snapshot>()
			.new_manager<cworld, sworld_snapshot>();

		//- Add services and layers of the game
		cfg(e);

		if (!e.init())
		{
			return -1;
		}

		e.run();
		e.shutdown();
		return 0;
	}

} //- kokoro::entry