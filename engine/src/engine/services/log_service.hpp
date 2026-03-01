#pragma once
#include <engine/iservice.hpp>
#include <cstdint>
#include <memory>

namespace kokoro
{
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
		void		trace(const char* string) { log(level_trace, string); }
		void		debug(const char* string) { log(level_debug, string); }
		void		info(const char* string) { log(level_info, string); }
		void		warn(const char* string) { log(level_warn, string); }
		void		err(const char* string) { log(level_err, string); }
		void		critical(const char* string) { log(level_critical, string); }

	private:
		std::unique_ptr<ilog_writer> m_emitter = nullptr;
	};

} //- kokoro