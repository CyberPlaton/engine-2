#pragma once
#include <memory>

namespace kokoro::core
{
	//------------------------------------------------------------------------------------------------------------------------
	class cmemory final
	{
	public:
		cmemory(char* data, unsigned size);
		~cmemory();

		cmemory& operator=(const cmemory&) = delete;
		cmemory(const cmemory&) = delete;
		cmemory(cmemory&&) = delete;
		cmemory& operator=(const cmemory&&) = delete;

		char*		data() const { return m_data; }
		unsigned	size() const { return m_size; }
		bool		empty() const { return size() == 0; }
		auto		begin() { return m_data; }
		auto		end() { return &m_data[m_size]; }

	private:
		char* m_data	= nullptr;
		unsigned m_size	= 0;
	};

	using memory_ref_t = std::shared_ptr<cmemory>;

} //- kokoro::core