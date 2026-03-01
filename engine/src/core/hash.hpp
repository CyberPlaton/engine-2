#pragma once
#include <string_view>

namespace kokoro::core
{
	namespace detail
	{
		namespace
		{
			constexpr uint64_t C_HASH = 0xcbf29ce484222325ull;
			constexpr uint64_t C_PRIME = 0x100000001b3ull;

		} //- unnamed

		//------------------------------------------------------------------------------------------------------------------------
		constexpr uint64_t fnv1a(const void* data, uint64_t size)
		{
			const char* d = (const char*)data;

			uint64_t hash = C_HASH;
			uint64_t prime = C_PRIME;

			for (auto i = 0ull; i < size; ++i)
			{
				const auto& c = d[i];
				hash ^= static_cast<uint64_t>(c);
				hash *= prime;
			}
			return hash;
		}

	} //- detail

	//------------------------------------------------------------------------------------------------------------------------
	constexpr uint64_t hash(std::string_view string)
	{
		return detail::fnv1a(string.data(), string.length());
	}

} //- kokoro::core