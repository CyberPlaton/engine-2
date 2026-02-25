#pragma once
#include <rttr.h>
#include <functional>
#include <string>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class ceffect_parser final
	{
	public:
		using include_handler_t = std::function<std::string(const char*)>;
		using meta_data_t = std::unordered_map<std::string, rttr::variant>;
		using function_meta_t = std::unordered_map<std::string, meta_data_t>;

		struct soutput
		{
			std::string m_vs;
			std::string m_ps;
			std::string m_cs;
		};

		ceffect_parser(const char* source, include_handler_t handler = nullptr);
		~ceffect_parser() = default;

		soutput		parse();

	private:
		enum scope : uint8_t
		{
			scope_global = 0,
			scope_vertex,
			scope_pixel,
			scope_compute,
		};

		std::string m_vs;
		std::string m_ps;
		std::string m_cs;
		const char* m_source = nullptr;
		include_handler_t m_handler = nullptr;
		function_meta_t* m_meta = nullptr;
		scope m_scope = scope_global;

	private:
		void write(const char* text);
		void scope_begin(scope s);
		void scope_end();
		void handle_include(int& i, std::vector<std::string>& lines, std::string_view path);
		void handle_annotations(int& i, std::vector<std::string>& lines);
	};

} //- kokoro