#include <engine/services/log_service.hpp>
#include <spdlog.h>

namespace kokoro
{
	namespace
	{
		//------------------------------------------------------------------------------------------------------------------------
		class cspdlog_writer final : public clog_service::ilog_writer
		{
		public:
			cspdlog_writer()
			{
#if DEBUG
				spdlog::flush_every(std::chrono::seconds(1));
				m_logger = spdlog::stdout_color_mt("debug");
#elif HYBRID
				spdlog::flush_every(std::chrono::seconds(1));
				m_logger = spdlog::stdout_color_mt("hybrid");
#elif RELEASE
				spdlog::flush_every(std::chrono::seconds(5));
				m_logger = spdlog::basic_logger_mt("release", "Log.log");
#endif
			}
			~cspdlog_writer() = default;

			void write(clog_service::level l, const char* string) override final
			{
				switch (l)
				{
				case clog_service::level_trace:
					m_logger->trace(string);
					break;
				case clog_service::level_debug:
					m_logger->debug(string);
					break;
				case clog_service::level_info:
					m_logger->info(string);
					break;
				case clog_service::level_warn:
					m_logger->warn(string);
					break;
				case clog_service::level_err:
					m_logger->error(string);
					break;
				case clog_service::level_critical:
					m_logger->critical(string);
					break;
				case clog_service::level_off:
					break;
				}
			}

			std::shared_ptr<spdlog::logger> m_logger;
		};

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	bool clog_service::init()
	{
		spdlog::set_pattern("%^[%n] %v%$");
		m_emitter = std::make_unique<cspdlog_writer>();
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void clog_service::post_init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void clog_service::shutdown()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void clog_service::update(float dt)
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void clog_service::set_level(level l)
	{
		spdlog::flush_on((spdlog::level::level_enum)l);
		spdlog::set_level((spdlog::level::level_enum)l);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void clog_service::log(level l, const char* string)
	{
		m_emitter->write(l, string);
	}

} //- kokoro