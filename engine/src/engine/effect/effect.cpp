#include <engine/effect/effect.hpp>
#include <engine/effect/effect_parser.hpp>
#include <unordered_map>

namespace kokoro
{
	namespace
	{
		std::unordered_map<std::string, seffect> cache;

		//------------------------------------------------------------------------------------------------------------------------
		std::string default_include_handler(const char* filepath)
		{
			fs::cfilepath path(filepath);

			if (!path.exists())
			{
				auto& vfs = cengine::instance().service<fs::cvirtual_filesystem>();

				if (const auto file_scope = vfs.open_file(path, core::file_mode_read); file_scope)
				{
					path = file_scope.file()->path();
				}
			}

			if (path.exists())
			{
				//- Load shader file data
				if (auto future = fs::load_text_from_file_async(path.view()); future.valid())
				{
					{
						core::casync_wait_scope wait_scope(future);
					}

					auto mem = future.get();

					if (mem && !mem->empty())
					{
						return std::string(mem->data(), mem->size());
					}
				}
			}

			return {};
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	seffect* create_from_file(std::string_view filepath)
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	seffect* create_from_memory(std::string_view name, const char* data, uint64_t size)
	{
		ceffect_parser parser(data, )
	}

	//------------------------------------------------------------------------------------------------------------------------
	void destroy(std::string_view filepath_or_name)
	{

	}

} //- kokoro