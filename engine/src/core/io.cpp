#include <core/io.hpp>

namespace kokoro
{
	bool write_variant(const rttr::variant& var, nlohmann::json& json);
	rttr::variant from_json_recursively(rttr::type expected, const nlohmann::json& json);
	void from_json_recursively(rttr::variant& object_out, rttr::type expected, const nlohmann::json& json);

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TNumeric>
	rttr::variant extract_numeric_type(TNumeric value, rttr::type required_type)
	{
		if (rttr::type::get<float>() == required_type) return static_cast<float>(value);
		else if (rttr::type::get<double>() == required_type) return static_cast<double>(value);
		else if (rttr::type::get<int64_t>() == required_type) return static_cast<int64_t>(value);
		else if (rttr::type::get<int32_t>() == required_type) return static_cast<int32_t>(value);
		else if (rttr::type::get<int16_t>() == required_type) return static_cast<int16_t>(value);
		else if (rttr::type::get<int8_t>() == required_type) return static_cast<int8_t>(value);
		else if (rttr::type::get<uint64_t>() == required_type) return static_cast<uint64_t>(value);
		else if (rttr::type::get<uint32_t>() == required_type) return static_cast<uint32_t>(value);
		else if (rttr::type::get<uint16_t>() == required_type) return static_cast<uint16_t>(value);
		else if (rttr::type::get<uint8_t>() == required_type) return static_cast<uint8_t>(value);
		else if (rttr::type::get<char>() == required_type) return static_cast<char>(value);
		else if (rttr::type::get<unsigned char>() == required_type) return static_cast<unsigned char>(value);
		else if (rttr::type::get<bool>() == required_type) return static_cast<bool>(value);
		return value;
	}

	//------------------------------------------------------------------------------------------------------------------------
	rttr::variant construct_object(rttr::type type)
	{
		if (type.get_constructor().is_valid())
		{
			return type.get_constructor().invoke();
		}
		if (auto var = type.create(); var.is_valid())
		{
			return var;
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	void extract_from_array(const rttr::variant& object, const nlohmann::json& json)
	{
		auto type = object.get_type();

		//- vector/array
		if (type.is_sequential_container())
		{
			auto view = object.create_sequential_view();
			auto expected = view.get_value_type();

			view.set_size(json.size());

			std::size_t i = 0;
			for (const auto& element : json)
			{
				auto val = from_json_recursively(expected, element);

				if (!view.set_value(i++, std::move(val)))
				{
				}
			}
		}
		//- map
		else if (type.is_associative_container())
		{
			auto view = object.create_associative_view();
			auto key_expected = view.get_key_type();
			auto val_expected = view.get_value_type();

			for (const auto& it : json)
			{
				if (!it.contains(C_MAP_KEY_NAME) || !it.contains(C_MAP_VALUE_NAME))
				{
					continue;
				}

				const auto& key_elem = it.at(C_MAP_KEY_NAME);
				const auto& val_elem = it.at(C_MAP_VALUE_NAME);

				auto key = from_json_recursively(key_expected, key_elem);
				auto val = from_json_recursively(val_expected, val_elem);

				if (!view.insert(key, val).second)
				{
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	rttr::variant extract_from_string(std::string_view value, const rttr::type& type)
	{
		rttr::variant var = std::string(value);

		if (var.get_type() == type || rttr::type::get<std::string_view>() == type)
		{
			return var;
		}
		//- try converting to desired type
		if (var.convert(type))
		{
			return var;
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	void extract_from_object(rttr::variant& object_out, const nlohmann::json& json)
	{
		auto type = object_out.get_type();

		if (type.is_class())
		{
			for (const auto& prop : type.get_properties())
			{
				auto name = prop.get_name().data();
				auto prop_type = prop.get_type().get_name().data();

				if (!json.contains(name))
				{
					continue;
				}

				rttr::variant object = from_json_recursively(prop.get_type(), json[name]);

				if (!prop.set_value(object_out, object))
				{
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	rttr::variant from_json_recursively(rttr::type expected, const nlohmann::json& json)
	{
		rttr::variant var;
		from_json_recursively(var, expected, json);
		return var;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void from_json_recursively(rttr::variant& object_out, rttr::type expected, const nlohmann::json& json)
	{
		if (json.is_null() || json.is_discarded())
		{
			object_out = {};
			return;
		}

		if (json.is_object())
		{
			if (!object_out)
			{
				if (rttr::type::get<rttr::variant>() == expected)
				{
					//- construct from expected type name defined in JSON
					if (json.contains(C_OBJECT_TYPE_NAME))
					{
						if (const auto variant_type = rttr::type::get_by_name(json[C_OBJECT_TYPE_NAME].get<std::string>()); variant_type.is_valid())
						{
							object_out = construct_object(variant_type);
						}
					}
				}
				else
				{
					object_out = construct_object(expected);
				}
			}

			//- construct from expected type name defined in JSON
			if (json.contains(C_OBJECT_TYPE_NAME))
			{
				const auto variant_type = json[C_OBJECT_TYPE_NAME].get<std::string>();

				extract_from_object(object_out, json[variant_type]);
			}
			else
			{
				extract_from_object(object_out, json.get<nlohmann::json::object_t>());
			}
		}
		else if (json.is_array())
		{
			if (!object_out)
			{
				object_out = construct_object(expected);
			}
			extract_from_array(object_out, json.get<nlohmann::json::array_t>());
		}
		else if (json.is_string())
		{
			if (expected.is_enumeration())
			{
			}
			object_out = extract_from_string(json.get<std::string>(), expected);

		}
		else if (json.is_boolean())
		{
			object_out = extract_numeric_type(json.get<bool>(), expected);
		}
		else if (json.is_number_float())
		{
			object_out = extract_numeric_type(json.get<double>(), expected);
		}
		else if (json.is_number_integer())
		{
			object_out = extract_numeric_type(json.get<int64_t>(), expected);
		}
		else if (json.is_number_unsigned())
		{
			object_out = extract_numeric_type(json.get<uint64_t>(), expected);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	rttr::variant from_json_blob(rttr::type expected, const char* data, unsigned size)
	{
		if (auto json = nlohmann::json::parse(data, data + size, nullptr, false, true); !json.is_discarded())
		{
			return from_json_object(expected, json);
		}
		return {};
	}

	//------------------------------------------------------------------------------------------------------------------------
	rttr::variant from_json_object(rttr::type expected, const nlohmann::json& json)
	{
		//- Proceed loading from update or up-to-date json
		rttr::variant object;
		from_json_recursively(object, expected, json);
		return object;
	}

	//------------------------------------------------------------------------------------------------------------------------
	rttr::variant from_json_string(rttr::type expected, std::string_view json)
	{
		return from_json_blob(expected, (const char*)json.data(), json.length());
	}

	//------------------------------------------------------------------------------------------------------------------------
	void to_json_recursive(const rttr::instance& object, nlohmann::json& json)
	{
		//-- wrap into object with type information
		//-- Note: type information is outside of the object to make deserialization easier
		json[C_OBJECT_TYPE_NAME] = object.get_type().get_raw_type().get_name().data();
		json = nlohmann::json::object();

		auto inst = object.get_type().get_raw_type().is_wrapper() ? object.get_wrapped_instance() : object;

		for (const auto prop : inst.get_derived_type().get_properties())
		{
			auto val = prop.get_value(inst);
			auto name = prop.get_name();

			if (val)
			{
				write_variant(val, json[name.data()]);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool is_fundamental_type(const rttr::type& type)
	{
		return type.is_arithmetic()
			|| type.is_enumeration()
			|| type == rttr::type::get<std::string>()
			|| type == rttr::type::get<std::string_view>()
			|| type == rttr::type::get<const char*>();
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool write_fundamental_type(const rttr::type& type, const rttr::variant& var, nlohmann::json& json)
	{
		if (type.is_arithmetic())
		{
			if (rttr::type::get<float>() == type) json = var.to_double();
			else if (rttr::type::get<double>() == type) json = var.to_double();
			else if (rttr::type::get<int64_t>() == type) json = var.to_int();
			else if (rttr::type::get<int32_t>() == type) json = var.to_int();
			else if (rttr::type::get<int16_t>() == type) json = var.to_int();
			else if (rttr::type::get<int8_t>() == type) json = var.to_int();
			else if (rttr::type::get<uint64_t>() == type) json = var.to_uint64();
			else if (rttr::type::get<uint32_t>() == type) json = var.to_uint64();
			else if (rttr::type::get<uint16_t>() == type) json = var.to_uint64();
			else if (rttr::type::get<uint8_t>() == type) json = var.to_uint64();
			else if (rttr::type::get<char>() == type) json = var.to_bool();
			else if (rttr::type::get<bool>() == type) json = var.to_bool();
			else
			{
				return false;
			}
			return true;
		}
		else if (type.is_enumeration())
		{
			//- try serializing as string
			bool result = false;
			if (auto s = var.to_string(&result); result)
			{
				json = s;
				return true;
			}
			//- could not write enum value, ignore but report
			json = nullptr;
			return true;
		}
		else if (rttr::type::get<std::string>() == type ||
			rttr::type::get<std::string_view>() == type ||
			rttr::type::get<const char*>() == type)
		{
			json = var.to_string();
			return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void write_array(const rttr::variant_sequential_view& view, nlohmann::json& json)
	{
		json = nlohmann::json::array();

		auto i = 0;
		for (const auto& var : view)
		{
			write_variant(var, json[i++]);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void write_map(const rttr::variant_associative_view& view, nlohmann::json& json)
	{
		json = nlohmann::json::array();

		auto i = 0;
		if (!view.is_key_only_type())
		{
			for (const auto& pair : view)
			{
				write_variant(pair.first, json[i][C_MAP_KEY_NAME.data()]);
				write_variant(pair.second, json[i][C_MAP_VALUE_NAME.data()]);
				++i;
			}
		}
		else
		{
			for (const auto& var : view)
			{
				write_variant(var.first, json[i++]);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool write_variant(const rttr::variant& var, nlohmann::json& json)
	{
		auto v = var;
		auto t = var.get_type();

		if (t.is_wrapper())
		{
			//- extract wrapped type
			t = t.get_wrapped_type();
			v = v.extract_wrapped_value();
		}

		//- number/bool/string/enum
		if (is_fundamental_type(t) && write_fundamental_type(t, v, json))
		{
			return true;
		}
		//- vector/array
		else if (t.is_sequential_container())
		{
			write_array(v.create_sequential_view(), json);
		}
		//- map
		else if (t.is_associative_container())
		{
			write_map(v.create_associative_view(), json);
		}
		else
		{
			//- write an object
			json = nlohmann::json::object();
			json[C_OBJECT_TYPE_NAME] = t.get_name().data();

			if (auto props = t.get_properties(); !props.empty())
			{
				to_json_recursive(v, json[t.get_name().data()]);
				return true;
			}
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	std::string to_json_string(rttr::instance object, bool beautify /*= true*/)
	{
		auto json = to_json_object(object);

		return json.dump(beautify ? 4 : -1);
	}

	//------------------------------------------------------------------------------------------------------------------------
	nlohmann::json to_json_object(rttr::instance object)
	{
		nlohmann::json json;

		if (object.is_valid())
		{
			to_json_recursive(object, json);
		}

		return std::move(json);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void to_json_object(rttr::instance object, nlohmann::json& json)
	{
		if (object.is_valid())
		{
			to_json_recursive(object, json);
		}
	}

} //- kokoro