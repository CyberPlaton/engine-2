#pragma once
#include <rttr.h>

namespace rttr
{
	namespace detail
	{
		template<class T, typename = void> struct skey_type { using type = void; };
		template<class T, typename = void> struct svalue_type { using type = void; };
		template<class T> struct skey_type<T, std::void_t<typename T::key_type>> { using type = typename T::key_type; };
		template<class T> struct svalue_type<T, std::void_t<typename T::value_type>> { using type = typename T::value_type; };
		template<class T, typename = void> struct sis_iterator : std::false_type { };
		template<class T> struct sis_iterator<T, std::void_t<typename T::iterator>> : std::true_type { };
		template<class T> constexpr bool sis_container = sis_iterator<T>::value && !std::is_same_v<T, std::string>;
		struct no_default {};

		//------------------------------------------------------------------------------------------------------------------------
		enum constructor_policy : uint8_t
		{
			constructor_policy_none = 0,
			constructor_policy_as_object,
			constructor_policy_as_shared_ptr,
			constructor_policy_as_raw_ptr,
			constructor_policy_no_default,
		};

		//- Utility function to retrieve metadata for a globally registered property, like
		//- rttr::registration::property("global_setting", &some_global_var)
		//-		(rttr::metadata("MyKey", "MyValue"));
		//------------------------------------------------------------------------------------------------------------------------
		template<typename TKey, typename TValue>
		static TValue global_property_meta(rttr::string_view prop, TKey key)
		{
			auto property = rttr::type::get_global_property(prop);

			auto value = property.get_metadata(key);

			return value.template get_value<TValue>();
		}

		//------------------------------------------------------------------------------------------------------------------------
		template<typename TValue>
		static TValue global_property_meta(rttr::string_view prop, rttr::string_view key)
		{
			return global_property_meta<rttr::string_view, TValue>(prop, key);
		}

		//- Utility function to retrieve metadata for a class member property, like
		//- rttr::csettings<scamera_settings>("scamera_settings")
		//-		.prop("m_movement_speed", &scamera_settings::m_movement_speed,
		//-			rttr::metadata("Description", "Speed"),
		//-			rttr::metadata("Name", "Speed"));
		//------------------------------------------------------------------------------------------------------------------------
		template<typename TClass, typename TKey, typename TValue>
		static TValue class_member_property_meta(rttr::string_view prop_name, TKey key)
		{
			rttr::type type = rttr::type::get<TClass>();

			if (const auto property = type.get_property(prop_name); property.is_valid())
			{
				if (const auto meta = property.get_metadata(key); meta.is_valid())
				{
					return meta.template get_value<TValue>();
				}
			}
			return {};
		}

		//------------------------------------------------------------------------------------------------------------------------
		static rttr::variant class_meta(const rttr::type& type, std::string_view name)
		{
			if (auto meta = type.get_metadata(rttr::string_view(name.data())); meta.is_valid())
			{
				return meta;
			}
			return {};
		}

		//------------------------------------------------------------------------------------------------------------------------
		template<typename TClass, typename TKey, typename TValue>
		static TValue class_meta(TKey key)
		{
			rttr::type type = rttr::type::get<TClass>();

			return class_meta(type, key).template get_value<TValue>();
		}

		//------------------------------------------------------------------------------------------------------------------------
		static rttr::method class_method(const rttr::type& type, std::string_view name)
		{
			return type.get_method(name.data());
		}

		//- Utility function to retrieve a method registered for class TClass.
		//------------------------------------------------------------------------------------------------------------------------
		template<typename TClass>
		static rttr::method class_method(std::string_view name)
		{
			return get_method(rttr::type::get<TClass>(), name);
		}

		//- Register a default constructor for a class. Can be container types, such as vector_t<> or normal classes.
		//------------------------------------------------------------------------------------------------------------------------
		template<typename TType>
		static void default_constructor()
		{
			//- i.e. vector_t or array_t (c-style arrays are not supported)
			if constexpr (!std::is_array_v<TType>)
			{
				if (const auto type = rttr::type::get<TType>(); type.is_valid() && type.get_constructors().empty())
				{
					typename rttr::registration::template class_<TType> __class(type.get_name());
					__class.constructor()(rttr::policy::ctor::as_object);
				}
			}
			//- recursive register default constructor for i.e. map_t or nested containers, i.e. vector_t< map_t<>> etc.
			if constexpr (detail::sis_container<TType>)
			{
				using key_t = typename detail::skey_type<TType>::type;
				using value_t = typename detail::svalue_type<TType>::type;

				if constexpr (detail::sis_container<key_t>)
				{
					default_constructor<key_t>();
				}
				if constexpr (detail::sis_container<value_t>)
				{
					default_constructor<value_t>();
				}
			}
		}

	} //- detail

	//- Utility class for RTTR registration. Adds a default constructor.
	//- Intended for classes. Use the class_() function to register metas etc.
	//- We do not check for duplicate registrations as those might be a side-effect of REFLECT_INLINE() usage,
	//- just be aware that the latest registration can override a previous one with same type name.
	//-
	//- TPolicy can be one of
	//-		- rttr::detail::as_object
	//-		- rttr::detail::as_std_shared_ptr
	//-		- rttr::detail::as_raw_pointer
	//-		- rttr::detail::no_default (omits registering a default constructor, useful when you explicitly don\B4t want one)
	//------------------------------------------------------------------------------------------------------------------------
	template<class TClass, typename TPolicy = rttr::detail::as_object>
	class cregistrator
	{
	public:
		cregistrator(rttr::string_view name);

		template<typename TMethod, typename... TMeta>
		cregistrator& meth(rttr::string_view name, TMethod method, TMeta&&... metadata);

		template<typename TProp, typename... TMeta>
		cregistrator& prop(rttr::string_view name, TProp property, TMeta&&... metadata);

		template<typename TKey, typename TValue>
		cregistrator& meta(TKey key, TValue value);

		template<typename TValue>
		cregistrator& meta(rttr::string_view name, TValue value);

		template<typename... ARGS>
		cregistrator& ctor(detail::constructor_policy policy = detail::constructor_policy::constructor_policy_as_object)
		{
			auto c = m_object.template constructor<ARGS...>();

			switch (policy)
			{
			default:
			case detail::constructor_policy_none:
			case detail::constructor_policy_as_object:
			{
				c(rttr::policy::ctor::as_object);
				break;
			}
			case detail::constructor_policy_as_shared_ptr:
			{
				c(rttr::policy::ctor::as_std_shared_ptr);
				break;
			}
			case detail::constructor_policy_as_raw_ptr:
			{
				c(rttr::policy::ctor::as_raw_ptr);
				break;
			}
			}
			return *this;
		}

	private:
		rttr::registration::class_<TClass> m_object;
	};

	//- Note: when registering string like meta, we convert the string like object to rttr::string_view, meaning retrieving it
	//- can be done using rttr::string_view for key and the value is extractable using template get_value<rttr::string_view>();
	//------------------------------------------------------------------------------------------------------------------------
	template<class TClass, typename TPolicy /*= rttr::detail::as_object*/>
	template<typename TKey, typename TValue>
	cregistrator<TClass, TPolicy>& rttr::cregistrator<TClass, TPolicy>::meta(TKey key, TValue value)
	{
		const auto opt_convert_to_string = [](auto&& input) -> decltype(auto)
			{
				using U = std::decay_t<decltype(input)>;

				if constexpr (std::is_same_v<U, std::string_view> ||
					std::is_same_v<U, rttr::string_view> ||
					std::is_same_v<U, std::string>)
				{
					return rttr::string_view(input.data());
				}
				else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>)
				{
					return rttr::string_view(input);
				}
				else
				{
					return std::forward<decltype(input)>(input);
				}
			};

		this->m_object
		(
			rttr::metadata(opt_convert_to_string(key), opt_convert_to_string(value))
		);
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<class TClass, typename TPolicy /*= rttr::detail::as_object*/>
	rttr::cregistrator<TClass, TPolicy>::cregistrator(rttr::string_view name) :
		m_object(name)
	{
		if constexpr (!std::is_same_v<rttr::detail::no_default, TPolicy>)
		{
			//- class should be registered with RTTR by this point
			if constexpr (std::is_same_v<rttr::detail::as_object, TPolicy>)
			{
				static_assert(std::is_copy_constructible_v<TClass>, "Invalid operation. Class must be copy-constructible when registered as with 'as_object' policy");

				m_object.constructor()(rttr::policy::ctor::as_object);
			}
			else if constexpr (std::is_same_v<rttr::detail::as_raw_pointer, TPolicy>)
			{
				m_object.constructor()(rttr::policy::ctor::as_raw_ptr);
			}
			else if constexpr (std::is_same_v<rttr::detail::as_std_shared_ptr, TPolicy>)
			{
				m_object.constructor()(rttr::policy::ctor::as_std_shared_ptr);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<class TClass, typename TPolicy /*= rttr::detail::as_object*/>
	template<typename TProp, typename... TMeta>
	cregistrator<TClass, TPolicy>& rttr::cregistrator<TClass, TPolicy>::prop(rttr::string_view name, TProp property, TMeta&&... metadata)
	{
		if constexpr (sizeof... (TMeta) > 0)
		{
			m_object.property(name, std::move(property))
				(
					std::forward<TMeta>(metadata)...
					);
		}
		else
		{
			m_object.property(name, std::move(property));
		}

		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<class TClass, typename TPolicy /*= rttr::detail::as_object*/>
	template<typename TMethod, typename... TMeta>
	cregistrator<TClass, TPolicy>& rttr::cregistrator<TClass, TPolicy>::meth(rttr::string_view name, TMethod method, TMeta&&... metadata)
	{
		if constexpr (sizeof... (TMeta) > 0)
		{
			m_object.method(name, std::move(method))
				(
					std::forward<TMeta>(metadata)...);
		}
		else
		{
			m_object.method(name, std::move(method));
		}

		return *this;
	}

	namespace detail
	{
		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		rttr::variant invoke_static_function(rttr::type class_type, rttr::string_view function_name, ARGS&&... args)
		{
			if (const auto m = class_type.get_method(function_name); m.is_valid())
			{
				return m.invoke({}, args...);
			}
			return {};
		}

		//------------------------------------------------------------------------------------------------------------------------
		template<typename... ARGS>
		rttr::variant invoke_function(const rttr::variant& var, rttr::string_view function_name, ARGS&&... args)
		{
			const auto class_type = var.get_type();

			if (const auto m = class_type.get_method(function_name); m.is_valid())
			{
				return m.invoke(var, std::forward<ARGS>(args)...);
			}
			return {};
		}

	} //- detail

} //- rttr
