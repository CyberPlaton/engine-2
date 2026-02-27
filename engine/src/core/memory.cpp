#include <core/memory.hpp>
#include <dlmalloc.h>

namespace kokoro::core
{
	//- Allocate necessary memory and copy provided data, freeing incomming data is not our responsibility
	//------------------------------------------------------------------------------------------------------------------------
	cmemory::cmemory(char* data, unsigned size)
	{
		m_data = (char*)KOKORO_MALLOC(size + 1);
		m_size = size;
		std::memcpy(m_data, data, size);
		m_data[size] = '\0';
	}

	//------------------------------------------------------------------------------------------------------------------------
	cmemory::~cmemory()
	{
		KOKORO_FREE(m_data);
		m_data = nullptr;
		m_size = 0;
	}

} //- kokoro::core