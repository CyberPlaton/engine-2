#include <engine/modules/postprocess_module.hpp>
#include <core/registrator.hpp>

namespace kokoro
{
	//-------------------------------------------------------------------------------------------------------------------------
	modules::sconfig spostprocess_module::config()
	{
		modules::sconfig cfg;
		cfg.m_name = "spostprocess_module";
		cfg.m_components = { "spostprocess_volume" };
		cfg.m_systems = { "spostprocess_gather_system", "spostprocess_submit_system" };
		cfg.m_modules = {};

		return cfg;
	}

} //- kokoro

RTTR_REGISTRATION
{
	using namespace kokoro;
	REGISTER_MODULE(spostprocess_module);
}