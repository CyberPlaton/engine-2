#pragma once
#include <nlohmann.h>
#include <rttr.h>
#include <string_view>


namespace kokoro::core
{
	constexpr std::string_view C_OBJECT_TYPE_NAME	= "__type__";
	constexpr std::string_view C_MAP_KEY_NAME		= "__key__";
	constexpr std::string_view C_MAP_VALUE_NAME		= "__value__";

	[[nodiscard]] rttr::variant		from_json_blob(rttr::type expected, const char* data, unsigned size);
	[[nodiscard]] rttr::variant		from_json_object(rttr::type expected, const nlohmann::json& json);
	[[nodiscard]] rttr::variant		from_json_string(rttr::type expected, std::string_view json);
	std::string						to_json_string(rttr::instance object, bool beautify = true);
	[[nodiscard]] nlohmann::json	to_json_object(rttr::instance object);
	void							to_json_object(rttr::instance object, nlohmann::json& json);

} //- kokoro::core