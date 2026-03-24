#pragma once
#include <engine/iservice.hpp>
#include <core/memory.hpp>
#include <core/mutex.hpp>
#include <cstdint>
#include <unordered_map>
#include <filesystem>
#include <future>

namespace kokoro
{
	using filepath_t = std::filesystem::path;

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
	class cfile final : public std::enable_shared_from_this<cfile>
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

	//- Class responsible for extending the lifetime of a file object within its scope, not meant to be stored and only
	//- used inside a function that uses file I/O operations.
	//------------------------------------------------------------------------------------------------------------------------
	class cfile_wrapper final
	{
	public:
		explicit cfile_wrapper(const std::shared_ptr<cfile>& file);
		cfile_wrapper() = default;
		~cfile_wrapper() = default;
		cfile_wrapper& operator=(const cfile_wrapper&) = delete;
		cfile_wrapper(const cfile_wrapper&) = delete;
		cfile_wrapper(cfile_wrapper&&) = delete;
		cfile_wrapper& operator=(const cfile_wrapper&&) = delete;

		inline bool				valid() const { return m_file != nullptr; }
		operator				bool() const { return valid(); }
		inline cfile&			get() { return *m_file.get(); }
		inline const cfile&		get() const { return *m_file.get(); }

	private:
		std::shared_ptr<cfile> m_file = nullptr;
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

		bool		exists(const filepath_t& filepath) const;
		auto		resolve(const filepath_t& filepath) const -> std::pair<bool, filepath_t>;
		auto		assign(std::string_view alias, std::string_view basepath) -> cvirtual_filesystem_service&;
		auto		open(const filepath_t& filepath, int file_mode) -> cfile_wrapper;
		std::string	basepath(std::string_view alias) const;

	private:
		std::unordered_map<std::string, std::string> m_aliases;
		std::unordered_map<std::string, std::shared_ptr<cfile>> m_files;
		mutable core::cmutex m_mutex;
	};

} //- kokoro