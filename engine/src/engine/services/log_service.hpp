#pragma once
#include <engine/iservice.hpp>
#include <engine.hpp>
#include <cstdint>
#include <memory>

namespace kokoro
{
	//- TODO: remove the ilog_writer and use spdlog directly
	//------------------------------------------------------------------------------------------------------------------------
	class clog_service final : public iservice
	{
	public:
		enum level : uint8_t
		{
			level_trace = 0,
			level_debug,
			level_info,
			level_warn,
			level_err,
			level_critical,
			level_off,
		};

		class ilog_writer
		{
		public:
			virtual void write(level, const char*) = 0;
		};

		clog_service() = default;
		~clog_service() = default;

		bool		init() override final;
		void		post_init() override final;
		void		shutdown() override final;
		void		update(float dt) override final;

		void		set_level(level l);
		void		log(level l, const char* string);
		template<typename... ARGS>
		void		log(level l, const char* format, ARGS&&... args) { m_emitter->write(l, fmt::format(format, std::forward<ARGS>(args)...).c_str()); }

		void		trace(const char* string) { log(level_trace, string); }
		void		debug(const char* string) { log(level_debug, string); }
		void		info(const char* string) { log(level_info, string); }
		void		warn(const char* string) { log(level_warn, string); }
		void		err(const char* string) { log(level_err, string); }
		void		critical(const char* string) { log(level_critical, string); }

		template<typename... ARGS>
		void		trace(const char* format, ARGS&&... args) { log(level_trace, format, std::forward<ARGS>(args)...); }
		template<typename... ARGS>
		void		debug(const char* format, ARGS&&... args) { log(level_debug, format, std::forward<ARGS>(args)...); }
		template<typename... ARGS>
		void		info(const char* format, ARGS&&... args) { log(level_info, format, std::forward<ARGS>(args)...); }
		template<typename... ARGS>
		void		warn(const char* format, ARGS&&... args) { log(level_warn, format, std::forward<ARGS>(args)...); }
		template<typename... ARGS>
		void		err(const char* format, ARGS&&... args) { log(level_err, format, std::forward<ARGS>(args)...); }
		template<typename... ARGS>
		void		critical(const char* format, ARGS&&... args) { log(level_critical, format, std::forward<ARGS>(args)...); }

	private:
		std::unique_ptr<ilog_writer> m_emitter = nullptr;
	};

	namespace log
	{
		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		void	trace(const char* format, ARGS&&... args)
		{
			instance().service<clog_service>().trace(format, std::forward<ARGS>(args)...);
		}

		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		void	debug(const char* format, ARGS&&... args)
		{
			instance().service<clog_service>().debug(format, std::forward<ARGS>(args)...);
		}

		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		void	info(const char* format, ARGS&&... args)
		{
			instance().service<clog_service>().info(format, std::forward<ARGS>(args)...);
		}

		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		void	warn(const char* format, ARGS&&... args)
		{
			instance().service<clog_service>().warn(format, std::forward<ARGS>(args)...);
		}

		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		void	err(const char* format, ARGS&&... args)
		{
			instance().service<clog_service>().err(format, std::forward<ARGS>(args)...);
		}

		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		void	critical(const char* format, ARGS&&... args)
		{
			instance().service<clog_service>().critical(format, std::forward<ARGS>(args)...);
		}

	} //- log

} //- kokoro