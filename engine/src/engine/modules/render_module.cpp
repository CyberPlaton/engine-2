#include <engine/modules/render_module.hpp>
#include <core/registrator.hpp>

namespace kokoro
{
	//-------------------------------------------------------------------------------------------------------------------------
	modules::sconfig srender_module::config()
	{
		modules::sconfig cfg;
		cfg.m_name = "srender_module";
		cfg.m_components = {};
		cfg.m_systems = {};
		cfg.m_modules = {};

		return cfg;
	}

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro;
	REGISTER_MODULE(srender_module);
}