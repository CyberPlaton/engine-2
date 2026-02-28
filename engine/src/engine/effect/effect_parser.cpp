#include <engine/effect/effect_parser.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine.hpp>
#include <fmt.h>
#include <sstream>

namespace kokoro
{
	namespace
	{
		//------------------------------------------------------------------------------------------------------------------------
		std::string default_include_handler(const char* filepath)
		{
			if (filepath_exists(filepath))
			{
				filepath_t path(filepath);
				auto& vfs = instance().service<cvirtual_filesystem_service>();

				if (auto file = vfs.open(path, file_options_read | file_options_text); file)
				{
					auto future = file->read_async();

					while (future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::ready) {}
					file->close();

					auto mem = future.get();

					if (mem && !mem->empty())
					{
						return std::string(mem->data(), mem->size());
					}
				}
			}
			return {};
		}

		//------------------------------------------------------------------------------------------------------------------------
		void split(const std::string& string, char delimiter, std::vector<std::string>& storage)
		{
			std::stringstream stream(string);
			std::string token;
			while (std::getline(stream, token, delimiter)) { storage.emplace_back(token); }
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
		rttr::type to_rttr_type(const std::string& type_string)
		{
			if (type_string == "string")
			{
				return rttr::type::get<std::string>();
			}
			return rttr::type::get_by_name(type_string);
		}

		//------------------------------------------------------------------------------------------------------------------------
		bool convert_string_to_value(rttr::variant& out, const std::string& value_string, rttr::type value_type)
		{
			if (value_type == rttr::type::get<int>())
			{
				out = std::stoi(value_string);
				return true;
			}
			else if (value_type == rttr::type::get<float>())
			{
				out = std::stof(value_string);
				return true;
			}
			else if (value_type == rttr::type::get<double>())
			{
				out = std::stod(value_string);
				return true;
			}
			else if (value_type == rttr::type::get<bool>())
			{
				out = (value_string == "true" || value_string == "1") ? true : false;
				return true;
			}
			else if (value_type == rttr::type::get<std::string>())
			{
				out = value_string;
				return true;
			}
			return false;
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	ceffect_parser::ceffect_parser(const char* source, include_handler_t handler /*= nullptr*/) :
		m_source(source),
		m_handler(handler ? handler : default_include_handler)
	{
	}

	//------------------------------------------------------------------------------------------------------------------------
	ceffect_parser::soutput ceffect_parser::parse()
	{
		soutput out;
		std::vector<std::string> lines;
		bool has_vertex_shader = false;
		bool has_pixel_shader = false;
		bool has_compute_shader = false;

		split(m_source, '\n', lines);

		auto i = 0;
		while (i < lines.size())
		{
			const auto& line = lines.at(i);

			if (starts_with(line, "#include"))
			{
				//- We handle two types of includes, "/path.fxh" and </path.fxh>
				auto start = line.find('"');
				auto end = line.rfind('"');

				if (start == std::string::npos)
				{
					start = line.find("<");
				}
				if (end == std::string::npos)
				{
					end = line.find(">");
				}

				if (start == std::string::npos || end == std::string::npos)
				{
					++i;
				}
				else if (start != std::string::npos && end != std::string::npos && end > start)
				{
					if (const auto path = line.substr(start + 1, end - start - 1); !path.empty())
					{
						handle_include(i, lines, path);
					}
				}
			}
			else if (starts_with(line, "@"))
			{
				const auto vertex_scope = line.find("VERTEX") != std::string::npos;
				const auto pixel_scope = line.find("PIXEL") != std::string::npos;
				const auto compute_scope = line.find("COMPUTE") != std::string::npos;
				const auto ending_scope = line.find("!") != std::string::npos;

				if (vertex_scope || pixel_scope || compute_scope)
				{
					if (m_scope == scope_global)
					{
						scope_begin(vertex_scope ? scope_vertex :
							pixel_scope ? scope_pixel : scope_compute);
					}
					else
					{
						scope_end();
					}
					++i;
				}
				else if (!(vertex_scope && pixel_scope && compute_scope) && ending_scope)
				{
					scope_end();
					++i;
				}
				//- We are beginning annotations/meta data for a function
				//else if (m_function_meta)
				//{
				//	handle_annotations(i, lines);
				//}
			}
			else
			{
				static std::string C_NEWLINE = "\n";

				//- Locate main function for current scope and validate usage
				if (const auto it = line.find("main"); it != std::string::npos)
				{
					switch (m_scope)
					{
					default:
					case scope_global:
						break;
					case scope_compute:
						has_compute_shader = true;
						break;
					case scope_pixel:
						has_pixel_shader = true;
						break;
					case scope_vertex:
						has_vertex_shader = true;
						break;
					}
				}

				if (!(line == C_NEWLINE))
				{
					write(line.c_str());
				}

				++i;
			}
		}

		out.m_vs = has_vertex_shader ? m_vs.c_str() : "";
		out.m_ps = has_pixel_shader ? m_ps.c_str() : "";
		out.m_cs = has_compute_shader ? m_cs.c_str() : "";
		return out;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ceffect_parser::write(const char* text)
	{
		const auto start = text;
		const auto end = text + strlen(text);

		switch (m_scope)
		{
		case scope_compute:
			m_cs.insert(m_cs.end(), start, end);
			break;
		case scope_vertex:
			m_vs.insert(m_vs.end(), start, end);
			break;
		case scope_pixel:
			m_ps.insert(m_ps.end(), start, end);
			break;
		default:
		case scope_global:
			m_cs.insert(m_cs.end(), start, end);
			m_vs.insert(m_vs.end(), start, end);
			m_ps.insert(m_ps.end(), start, end);
			break;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ceffect_parser::scope_begin(scope s)
	{
		m_scope = s;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ceffect_parser::scope_end()
	{
		m_scope = scope_global;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ceffect_parser::handle_include(int& i, std::vector<std::string>& lines, std::string_view path)
	{
		//- Ensure path has required format
		std::string filepath = starts_with(path, "/") ? path.data() : fmt::format("/{}", path.data());

		std::string source = m_handler(filepath.c_str());

		std::vector<std::string> out;

		split(source, '\n', out);

		lines.erase(lines.begin() + i);

		lines.insert(lines.begin() + i, out.begin(), out.end());
	}

	//------------------------------------------------------------------------------------------------------------------------
	void ceffect_parser::handle_annotations(int& i, std::vector<std::string>& lines)
	{
		const auto& func_line = lines.at(i - 1);

		const auto start = func_line.find_first_of(" ");
		const auto end = func_line.find("(");

		auto& function_meta = (*m_meta)[func_line.substr(start + 1, end - start - 1)];

		//- Process meta data until the end of annotations scope
		for (auto j = i + 1; j < lines.size(); ++j)
		{
			const auto& annotation_line = lines.at(j);

			if (starts_with(annotation_line, "@"))
			{
				i = j + 1;
				break;
			}

			//- Process and store annotation
			const auto type_end = annotation_line.find_first_of(" ");
			const auto equal_position = annotation_line.find_first_of("=");

			std::string type_string = annotation_line.substr(0, type_end);
			std::string key_string = annotation_line.substr(type_end + 1, equal_position - type_end - 1);
			std::string value_string = annotation_line.substr(equal_position + 1);

			type_string.erase(0, type_string.find_first_not_of(" \t"));
			type_string.erase(type_string.find_last_not_of(" \t") + 1);

			key_string.erase(0, key_string.find_first_not_of(" \t"));
			key_string.erase(key_string.find_last_not_of(" \t") + 1);

			value_string.erase(0, value_string.find_first_not_of(" \t\""));
			value_string.erase(value_string.find_last_not_of(" \t\";") + 1);

			rttr::variant value;

			if (convert_string_to_value(value, value_string, to_rttr_type(type_string)))
			{
				function_meta[key_string] = value;
			}
		}
	}

} //- kokoro