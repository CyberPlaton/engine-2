#pragma once
#include <engine/iservice.hpp>
#include <core/memory.hpp>
#include <cstdint>
#include <unordered_map>
#include <filesystem>
#include <future>

namespace kokoro
{
	using filepath_t = std::filesystem::path;

	bool	filepath_exists(std::string_view filepath);		//- Query whether file or folder at given path exists

	//------------------------------------------------------------------------------------------------------------------------
	enum file_options : uint8_t
	{
		file_options_none		= 0,
		file_options_read		= 1 << 0,
		file_options_write		= 1 << 1,
		file_options_read_write	= file_options_read | file_options_write,
		file_options_append		= 1 << 3,
		file_options_truncate	= 1 << 4,
		file_options_text		= 1 << 5,	//- For loading JSON data and everything that is text
		file_options_binary		= 1 << 6,	//- For loading binary data such as images
	};

	//------------------------------------------------------------------------------------------------------------------------
	class cfile final
	{
	public:
		enum seek_origin : uint8_t
		{
			seek_origin_none = 0,
			seek_origin_begin,		//- Position relative from start of file (i.e. forwards)
			seek_origin_end,		//- Position relative from end of file (i.e. backwards)
			seek_origin_set			//- Position relative from current position in file (i.e. relative to cursor in file)
		};

		enum state : uint8_t
		{
			state_none		= 0,
			state_opened	= 1 << 0,
			state_read_only	= 1 << 1,
			state_has_changes= 1 << 2,
		};

		cfile(const filepath_t& filepath);
		~cfile();

		filepath_t	path();
		unsigned	size();
		bool		read_only();
		bool		opened();
		void		open(int file_mode);
		void		close();
		unsigned	tell();
		unsigned	read(char* buffer, unsigned datasize);
		auto		read_sync() -> core::memory_ref_t;
		auto		read_async() -> std::future<core::memory_ref_t>;
		unsigned	write(const char* buffer, unsigned datasize);
		unsigned	seek(int offset, seek_origin origin);
		bool		seek_to_start();
		bool		seek_to_end();
		bool		seek_to(unsigned offset);
		FILE*		native() { return m_file; }

	private:
		filepath_t m_filepath;
		FILE* m_file			= nullptr;
		int m_state				= 0;
		int m_mode				= 0;
	};

	//------------------------------------------------------------------------------------------------------------------------
	class cvirtual_filesystem_service final : public iservice
	{
	public:
		cvirtual_filesystem_service() = default;
		~cvirtual_filesystem_service() = default;

		bool		init() override final;
		void		post_init() override final;
		void		shutdown() override final;
		void		update(float dt) override final;

		auto		assign(std::string_view alias, std::string_view basepath) -> cvirtual_filesystem_service&;
		cfile*		open(const filepath_t& filepath, int file_mode);
		std::string	basepath(std::string_view alias) const;

	private:
		std::unordered_map<std::string, std::string> m_aliases;
		std::unordered_map<std::string, cfile> m_files;
	};

} //- kokoro