#pragma once
#include <array>
#include <string>
#include <rttr.h>

namespace kokoro::core
{
	//- Class implementing universally unique identifiers of the form "fe5501a1-4ed7-4062-a62f-32663892fea3".
	//------------------------------------------------------------------------------------------------------------------------
	class cuuid final
	{
	public:
		using bytes_t = std::array<unsigned char, 16u>;
		static const cuuid C_INVALID_UUID;
		static const std::array<unsigned char, 16u> C_INVALID_DATA;

		static std::string generate(size_t seed = 0);

		cuuid();
		cuuid(const bytes_t& data);
		cuuid(const std::string& uuid);
		cuuid(size_t seed);
		cuuid(const cuuid& other);
		~cuuid() = default;

		std::string string() const { return generate_string(); }
		uint64_t    hash() const;
		bool        is_equal_to(const cuuid& uuid) const { return compare(uuid) == 0; }
		bool        is_smaller_as(const cuuid& uuid) const { return compare(uuid) < 0; }
		bool        is_higher_as(const cuuid& uuid) const { return compare(uuid) > 0; }
		bool        operator<(const cuuid& uuid) const { return is_smaller_as(uuid); }
		bool        operator==(const cuuid& other) const { return is_equal_to(other); }
		bool        operator!=(const cuuid& other) const { return !is_equal_to(other); };

	private:
		inline static constexpr auto C_RANDOM_BYTES_COUNT = 4;
		inline static constexpr unsigned char C_HEX[] = "0123456789abcdef";

		std::array<unsigned char, 16u> m_data;

	private:
		void do_generate(size_t seed);
		void parse_string(const std::string& uuid, bytes_t& out);
		void write_string(const bytes_t& data, std::string& out) const;
		unsigned hex2dec(char c);
		void copy_to(cuuid& other);
		void copy_from(const cuuid& other);
		int compare(const cuuid& other) const;
		std::string generate_string() const;

		RTTR_ENABLE();
		RTTR_REGISTRATION_FRIEND;
	};

} //- kokoro::core

namespace std
{
	//- Specializations for use with std template structures
	//------------------------------------------------------------------------------------------------------------------------
	template<>
	struct hash<kokoro::core::cuuid>
	{
		std::size_t operator()(const kokoro::core::cuuid& uuid) const
		{
			return uuid.hash();
		}
	};
}
