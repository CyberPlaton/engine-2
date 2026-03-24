#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/thread_service.hpp>
#include <engine.hpp>
#include <dlmalloc.h>
#include <fmt.h>

namespace kokoro
{
	namespace
	{
		//- Note: truncate and text reading is the default
		//------------------------------------------------------------------------------------------------------------------------
		const char* to_openmode(int file_mode)
		{
			const char* mode = nullptr;

			const auto reading = !!(file_mode & file_options_read);
			const auto writing = !!(file_mode & file_options_write);
			const auto text = !!(file_mode & file_options_text);

			if (reading && writing)
			{
				mode = text ? "r+t" : "r+b";
			}
			else if (reading)
			{
				mode = text ? "rt" : "rb";
			}
			else if (writing)
			{
				mode = text ? "wt" : "wb";
			}

			return mode;
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool starts_with(std::string_view string, std::string_view substr)
		{
			if (string.length() >= substr.length())
			{
				return string.compare(0, substr.length(), substr.data()) == 0;
			}
			return false;
		}

		//------------------------------------------------------------------------------------------------------------------------
		std::string replace(const std::string& string, const std::string& substr, const std::string& replacement)
		{
			std::string out = string;

			if (const auto it = string.find(substr); it != std::string::npos)
			{
				out.replace(it, substr.length(), replacement);
			}

			return out;
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	cfile_wrapper::cfile_wrapper(const std::shared_ptr<cfile>& file) :
		m_file(file)
	{
	}

	//------------------------------------------------------------------------------------------------------------------------
	cfile::cfile(const filepath_t& filepath) :
		m_filepath(filepath),
		m_file(nullptr),
		m_state(state_read_only)
	{
	}

	//------------------------------------------------------------------------------------------------------------------------
	cfile::~cfile()
	{
		close();
	}

	//------------------------------------------------------------------------------------------------------------------------
	filepath_t cfile::path()
	{
		return m_filepath;
	}

	//------------------------------------------------------------------------------------------------------------------------
	unsigned cfile::size()
	{
		if (opened())
		{
			auto current = tell();
			seek(0, seek_origin_end);
			auto filesize = tell();
			seek(current, seek_origin_begin);

			return filesize;
		}
		return 0;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cfile::read_only()
	{
		return !!(state_read_only & m_state);
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cfile::opened()
	{
		return !!(state_opened & m_state);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cfile::open(int file_mode)
	{
		if (opened() && m_mode == file_mode)
		{
			seek(0, seek_origin_begin);
			return;
		}

		//- Set to text mode by default if none selected
		if (!(file_mode & file_options_text) && !(file_mode & file_options_binary))
		{
			file_mode |= file_options_text;
		}

		m_mode = file_mode;
		m_state = state_read_only;

		if (!!(m_mode & file_options_write) || !!(m_mode & file_options_truncate))
		{
			m_state &= ~state_read_only;
		}

		m_file = fopen(path().generic_string().c_str(), to_openmode(m_mode));

		if (m_file)
		{
			m_state |= state_opened;
		}
		seek_to_start();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cfile::close()
	{
		if (m_file)
		{
			fclose(m_file);
			m_file = nullptr;
		}
		m_state &= ~state_opened;
	}

	//------------------------------------------------------------------------------------------------------------------------
	unsigned cfile::tell()
	{
		if (opened())
		{
			return static_cast<unsigned>(ftell(m_file));
		}
		return 0;
	}

	//------------------------------------------------------------------------------------------------------------------------
	unsigned cfile::read(char* buffer, unsigned datasize)
	{
		if (!opened() || !buffer || datasize == 0)
		{
			return 0;
		}

		seek_to(0);

		auto finished_reading = fread(buffer, sizeof(char), datasize, m_file);

		if (finished_reading == datasize)
		{
			return datasize;
		}
		return static_cast<unsigned>(finished_reading);
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cfile::read_sync() -> core::memory_ref_t
	{
		if (!opened())
		{
			return nullptr;
		}

		core::memory_ref_t output = nullptr;
		char* text = nullptr;
		unsigned count = 0;

		//- Query the full file size
		fseek(m_file, 0, SEEK_END);
		const auto filesize = static_cast<unsigned>(ftell(m_file));
		fseek(m_file, 0, SEEK_SET);

		if (filesize > 0)
		{
			text = (char*)KOKORO_MALLOC((filesize + 1) * sizeof(char));

			if (text != nullptr)
			{
				count = static_cast<unsigned>(fread(text, sizeof(char), filesize, m_file));

				if (count < filesize)
				{
					text = (char*)KOKORO_REALLOC(text, count + 1);
				}
				text[count] = '\0';
			}
		}

		if (text)
		{
			output = std::make_shared<core::cmemory>(text, count);
			KOKORO_FREE(text);
		}

		return output;
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cfile::read_async() -> std::future<core::memory_ref_t>
	{
		auto& ts = instance().service<cthread_service>();

		return ts.async("file read_async", [this]() -> core::memory_ref_t
			{
				return read_sync();
			});
	}

	//------------------------------------------------------------------------------------------------------------------------
	unsigned cfile::write(const char* buffer, unsigned datasize)
	{
		if (!opened() || read_only())
		{
			return 0;
		}

		auto finished_writing = fwrite(buffer, sizeof(char), datasize, m_file);

		if (finished_writing == datasize)
		{
			return datasize;
		}
		return static_cast<unsigned>(finished_writing);
	}

	//------------------------------------------------------------------------------------------------------------------------
	unsigned cfile::seek(int offset, seek_origin origin)
	{
		if (!opened())
		{
			return 0;
		}

		int way = 0;

		switch (origin)
		{
		case seek_origin_begin:
			way = SEEK_SET;
			break;

		case seek_origin_end:
			way = SEEK_END;
			break;

		case seek_origin_set:
			way = SEEK_CUR;
			break;

		default:
		case seek_origin_none:
			return 0;
		}

		fseek(m_file, static_cast<long>(offset), way);
		return tell();
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cfile::seek_to_start()
	{
		if (opened())
		{
			return (fseek(m_file, 0, SEEK_SET) == 0);
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cfile::seek_to_end()
	{
		if (opened())
		{
			const auto file_size = size();

			return (fseek(m_file, file_size, SEEK_SET) == file_size);
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cfile::seek_to(unsigned offset)
	{
		if (opened())
		{
			const auto file_size = size();
			return (fseek(m_file, offset, SEEK_SET) == offset);
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cvirtual_filesystem_service::init()
	{
		assign("engine", KOKORO_RESOURCES_DIR);
		assign("/", PROJECT_RESOURCES_DIR);

		auto working_dir = std::filesystem::current_path();
		working_dir /= ".temp";
		std::filesystem::create_directory(working_dir);
		assign(".temp", working_dir.generic_string());

		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cvirtual_filesystem_service::post_init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cvirtual_filesystem_service::shutdown()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cvirtual_filesystem_service::update(float dt)
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	bool cvirtual_filesystem_service::exists(const filepath_t& filepath) const
	{
		std::error_code err;
		const auto e = std::filesystem::directory_entry(filepath, err);
		return e.exists();
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cvirtual_filesystem_service::resolve(const filepath_t& filepath) const -> std::pair<bool, filepath_t>
	{
		if (filepath.is_relative())
		{
			const auto filepath_string = filepath.generic_string();

			for (const auto& [alias, basepath] : m_aliases)
			{
				if (starts_with(filepath_string, alias))
				{
					return { true, filepath_t{ replace(filepath_string, alias, basepath) } };
				}
			}
		}

		return { exists(filepath), filepath };
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cvirtual_filesystem_service::assign(std::string_view alias, std::string_view basepath) -> cvirtual_filesystem_service&
	{
		m_aliases[alias.data()] = basepath.data();
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	auto cvirtual_filesystem_service::open(const filepath_t& filepath, int file_mode) -> cfile_wrapper
	{
		core::cscoped_mutex m(m_mutex);
		const auto filepath_string = filepath.generic_string();

		if (const auto it = m_files.find(filepath_string); it != m_files.end())
		{
			auto& file = it->second;
			file->open(file_mode | file_options_truncate);

			if (!file->opened())
			{
				//- Failed to open file for whatever reason, no need to keep it around
				m_files.erase(filepath_string);
				return {};
			}
			return cfile_wrapper{ it->second };
		}

		auto path = filepath;

		if (filepath.is_relative())
		{
			for (const auto& [alias, basepath] : m_aliases)
			{
				if (starts_with(filepath_string, alias))
				{
					path = filepath_t{ replace(filepath_string, alias, basepath) };
					break;
				}
			}
		}

		//- File stored under originally given path for easy resolve without resolving for aliases,
		//- the file itself is constructed using the real path for correct opening.
		if (auto [it, result] = m_files.emplace(std::piecewise_construct,
			std::forward_as_tuple(filepath_string),
			std::forward_as_tuple(new cfile(path))); result)
		{
			auto& file = it->second;
			file->open(file_mode | file_options_truncate);

			if (!file->opened())
			{
				//- Failed to open file for whatever reason, no need to keep it around
				m_files.erase(filepath_string);
			}
			else
			{
				return cfile_wrapper{ file };
			}
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cvirtual_filesystem_service::evict(const filepath_t& filepath)
	{
		core::cscoped_mutex m(m_mutex);
		m_files.erase(filepath.generic_string());
	}

	//------------------------------------------------------------------------------------------------------------------------
	std::string cvirtual_filesystem_service::basepath(std::string_view alias) const
	{
		if (const auto it = m_aliases.find(alias.data()); it != m_aliases.end())
		{
			return it->second;
		}
		return {};
	}

} //- kokoro