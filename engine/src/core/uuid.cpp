#include <core/uuid.hpp>
#include <core/registrator.hpp>
#include <core/hash.hpp>
#include <random>

namespace kokoro::core
{
	const cuuid::bytes_t cuuid::C_INVALID_DATA	= { 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f' };
	const cuuid cuuid::C_INVALID_UUID			= { C_INVALID_DATA };

	//------------------------------------------------------------------------------------------------------------------------
	cuuid::cuuid()
	{
		do_generate(std::random_device()());
	}

	//------------------------------------------------------------------------------------------------------------------------
	cuuid::cuuid(const std::string& uuid)
	{
		parse_string(uuid, m_data);
	}

	//------------------------------------------------------------------------------------------------------------------------
	cuuid::cuuid(size_t seed)
	{
		do_generate(seed);
	}

	//------------------------------------------------------------------------------------------------------------------------
	cuuid::cuuid(const cuuid& other)
	{
		copy_from(other);
	}

	//------------------------------------------------------------------------------------------------------------------------
	cuuid::cuuid(const bytes_t& data)
	{
		std::memcpy(&m_data[0], &data[0], sizeof(unsigned char) * 16);
	}

	//------------------------------------------------------------------------------------------------------------------------
	uint64_t cuuid::hash() const
	{
		return kokoro::core::hash(string());
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cuuid::do_generate(size_t seed)
	{
		//- setup random number generator
		static thread_local std::mt19937_64 random_engine;
		static thread_local bool seeded = false;
		if (!seeded)
		{
			random_engine.seed(seed);
			seeded = true;
		}
		std::uniform_int_distribution<std::mt19937_64::result_type> dist(std::numeric_limits<size_t>().min(), std::numeric_limits<size_t>().max());

		//- compute
		unsigned i, j, rnd;
		for (i = 0; i < (16 / C_RANDOM_BYTES_COUNT); ++i)
		{
			rnd = static_cast<uint32_t>(dist(random_engine));
			for (j = 0; j < C_RANDOM_BYTES_COUNT; ++j)
			{
				m_data[i * C_RANDOM_BYTES_COUNT + j] = (0xff & rnd >> (8 * j));
			}
		}
		//- set the version to 4
		m_data[6] = (m_data[6] & 0x0f) | 0x40;
		//- set the variant to 1 (a)
		m_data[8] = (m_data[8] & 0x0f) | 0xa0;
	}

	//------------------------------------------------------------------------------------------------------------------------
	std::string cuuid::generate(size_t seed /*= 0*/)
	{
		cuuid uuid;
		uuid.do_generate(seed == 0 ? std::random_device()() : seed);
		return uuid.string();
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cuuid::parse_string(const std::string& uuid, bytes_t& out)
	{
		unsigned i = 0, j = 0;
		while (i < 36 && j < 16)
		{
			if (uuid[i] != '-')
			{
				out[j++] = (hex2dec(uuid[i + 1]) << 4) | hex2dec(uuid[i]);
			}
			++i;
		}

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cuuid::write_string(const bytes_t& data, std::string& out) const
	{
		out.resize(36);
		unsigned i = 0, j = 0;
		while (j < 16)
		{
			if (j == 4 || j == 6 || j == 8 || j == 10)
			{
				out[i++] = '-';
			}
			out[i++] = C_HEX[(data[j] >> 4)];
			out[i++] = C_HEX[(0xf & data[j])];
			j++;
		}
		out[36] = '\0';
	}

	//------------------------------------------------------------------------------------------------------------------------
	unsigned cuuid::hex2dec(char c)
	{
		for (auto i = 0u; i < 16; ++i)
		{
			if (C_HEX[i] == c)
			{
				return i;
			}
		}
		return -1;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cuuid::copy_to(cuuid& other)
	{
		std::memcpy(&other.m_data[0], &m_data[0], 16);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cuuid::copy_from(const cuuid& other)
	{
		std::memcpy(&m_data[0], &other.m_data[0], 16);
	}

	//------------------------------------------------------------------------------------------------------------------------
	int cuuid::compare(const cuuid& other) const
	{
		return std::memcmp(&m_data[0], &other.m_data[0], 16);
	}

	//------------------------------------------------------------------------------------------------------------------------
	std::string cuuid::generate_string() const
	{
		std::string s;
		write_string(m_data, s);
		return s;
	}

} //- kokoro::core

RTTR_REGISTRATION
{
	using namespace kokoro::core;

	rttr::detail::default_constructor<cuuid::bytes_t>();

	//------------------------------------------------------------------------------------------------------------------------
	rttr::cregistrator<cuuid>("cuuid")
		.prop("m_data", &cuuid::m_data)
		;
}
