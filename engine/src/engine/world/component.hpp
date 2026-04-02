#pragma once
#include <core/io.hpp>
#include <core/registrator.hpp>
#include <rttr.h>
#include <nlohmann.h>
#include <string_view>

//-------------------------------------------------------------------------------------------------------------------------
#define __DECLARE_COMPONENT_IMPL__(c) \
c() = default; \
~c() = default; \
c(kokoro::cworld* w) { w->w().component<c>(); } \
static std::string_view name() { static constexpr std::string_view C_NAME = #c; return C_NAME; } \
static void do_default_serialize(const rttr::variant& var, nlohmann::json& json) \
{ \
if (const auto type = var.get_type(); type.is_valid() && type == rttr::type::get<c>()) \
{ \
const auto& comp = var.get_value<c>(); \
const auto type_name = rttr::type::get<c>().get_name().data(); \
json = nlohmann::json::object(); \
json[core::C_OBJECT_TYPE_NAME] = type_name; \
json[type_name] = core::to_json_object(comp); \
} \
} \
static void do_default_deserialize(rttr::variant& var, nlohmann::json& json) \
{ \
if (const auto type = rttr::type::get<c>(); type.is_valid() && var.is_valid() && type == var.get_type()) \
{ \
const auto type_name = type.get_name(); \
if (json.contains(type_name.data())) \
{ \
var = core::from_json_object(type, json[type_name.data()]); \
} \
} \
} \
static void do_serialize(const rttr::variant& var, nlohmann::json& json) \
{ \
const auto type = var.get_type(); \
if (auto custom_method = type.get_method(kokoro::ecs::C_COMPONENT_CUSTOM_SERIALIZE_FUNC_NAME.data()); custom_method.is_valid()) \
{ \
custom_method.invoke({}, var, json); \
} \
else if (auto default_method = type.get_method(kokoro::ecs::C_COMPONENT_DEFAULT_SERIALIZE_FUNC_NAME.data()); default_method.is_valid()) \
{ \
default_method.invoke({}, var, json); \
} \
else \
{ \
} \
} \
static void do_serialize(flecs::entity e, nlohmann::json& json) \
{ \
serialize(get(e), json); \
} \
static void do_deserialize(rttr::variant& var, nlohmann::json& json) \
{ \
if (!var.is_valid() || var.get_type() != rttr::type::get<c>()) \
{ \
var = c{}; \
} \
const auto type = rttr::type::get<c>(); \
if (json.contains(type.get_name().data())) \
{ \
if (auto custom_method = type.get_method(kokoro::ecs::C_COMPONENT_CUSTOM_DESERIALIZE_FUNC_NAME.data()); custom_method.is_valid()) \
{ \
custom_method.invoke({}, var, json); \
} \
else if (auto default_method = type.get_method(kokoro::ecs::C_COMPONENT_DEFAULT_DESERIALIZE_FUNC_NAME.data()); default_method.is_valid()) \
{ \
default_method.invoke({}, var, json); \
} \
else \
{ \
} \
} \
} \
static void do_deserialize(flecs::entity e, nlohmann::json& json) \
{ \
rttr::variant var; \
if (deserialize(var, json); var.is_valid()) \
{ \
set(e, var); \
} \
} \
static void serialize(const rttr::variant& var, nlohmann::json& json) \
{ \
do_serialize(var, json); \
} \
static void deserialize(rttr::variant& var, nlohmann::json& json) \
{ \
do_deserialize(var, json); \
} \
static void set(flecs::entity e, const rttr::variant& var) \
{ \
e.set<c>(var.get_value<c>()); \
} \
static void set_singleton(flecs::world w, const rttr::variant& var) \
{ \
w.set<c>(var.get_value<c>()); \
} \
static void add(flecs::entity e) \
{ \
e.add<c>(); \
} \
static void add_singleton(flecs::world w) \
{ \
w.add<c>(); \
} \
static void remove(flecs::entity e) \
{ \
e.remove<c>(); \
} \
static void remove_singleton(flecs::world w) \
{ \
w.remove<c>(); \
} \
static bool has(flecs::entity e) \
{ \
return e.has<c>(); \
} \
static void destroy_all(flecs::world& w) \
{ \
w.delete_with<c>(); \
} \
static rttr::variant get(flecs::entity e) \
{ \
return rttr::variant{ *e.get<c>() }; \
} \
static rttr::variant get_singleton(flecs::world w) \
{ \
return rttr::variant{ *w.get<c>() }; \
} \
RTTR_REGISTRATION_FRIEND; \
public:

//- Macro for defining a component. Note: when the component is constructed with the world as argument it registers itself
//- to flecs as a component, this is used in modules.
//- When serializing a component, you must acquire 'serialize' function and call it, the same for 'deserialize' when reading.
//-------------------------------------------------------------------------------------------------------------------------
#define DECLARE_COMPONENT(c) \
__DECLARE_COMPONENT_IMPL__(c) \
static bool show_ui(rttr::variant& comp);

//- Macro for defining a component that will not have UI functionality.
//-------------------------------------------------------------------------------------------------------------------------
#define DECLARE_COMPONENT_NO_UI(c) \
__DECLARE_COMPONENT_IMPL__(c)

//- Shortcut macro for defining a 'tag' component. A tag component does not have any data
//- and is useful in ecs queries and systems.
//- Note: declaring a tag requires registration to RTTR, otherwise serialization does not happen.
//-------------------------------------------------------------------------------------------------------------------------
#define DECLARE_TAG(c, ...) \
struct __VA_ARGS__ c final : public kokoro::ecs::icomponent \
{ \
	DECLARE_COMPONENT_NO_UI(c); \
	uint8_t dummy = 0; \
	RTTR_ENABLE(kokoro::ecs::icomponent); \
};

#define STRINGIFY(s) #s

//-- Use macro to register tag type to RTTR. Note that your tag is required to be inside 'tag' namespace.
//-------------------------------------------------------------------------------------------------------------------------
#define REGISTER_TAG(c) \
rttr::ccomponent<tag::c>(STRINGIFY(tag::c)) \
	.prop("dummy", &tag::c::dummy);

namespace kokoro::ecs
{
	static inline constexpr rttr::string_view C_COMPONENT_NAME_FUNC_NAME = "name";

	//- Public API for component read and write functions. When writing or reading acquire these
	//- functions and call them, no other.
	static inline constexpr rttr::string_view C_COMPONENT_SERIALIZE_FUNC_NAME = "serialize";
	static inline constexpr rttr::string_view C_COMPONENT_DESERIALIZE_FUNC_NAME = "deserialize";

	//- Custom implementation of converting a component to and from JSON.
	static inline constexpr rttr::string_view C_COMPONENT_CUSTOM_SERIALIZE_FUNC_NAME = "custom_serialize";
	static inline constexpr rttr::string_view C_COMPONENT_CUSTOM_DESERIALIZE_FUNC_NAME = "custom_deserialize";

	//- Default implementation of converting a component to and from JSON.
	static inline constexpr rttr::string_view C_COMPONENT_DEFAULT_SERIALIZE_FUNC_NAME = "do_default_serialize";
	static inline constexpr rttr::string_view C_COMPONENT_DEFAULT_DESERIALIZE_FUNC_NAME = "do_default_deserialize";

	static inline constexpr rttr::string_view C_COMPONENT_SET_FUNC_NAME = "set";
	static inline constexpr rttr::string_view C_COMPONENT_ADD_FUNC_NAME = "add";
	static inline constexpr rttr::string_view C_COMPONENT_REMOVE_FUNC_NAME = "remove";
	static inline constexpr rttr::string_view C_COMPONENT_ADD_SINGLETON_FUNC_NAME = "add_singleton";
	static inline constexpr rttr::string_view C_COMPONENT_REMOVE_SINGLETON_FUNC_NAME = "remove_singleton";
	static inline constexpr rttr::string_view C_COMPONENT_HAS_FUNC_NAME = "has";
	static inline constexpr rttr::string_view C_COMPONENT_SHOW_UI_FUNC_NAME = "show_ui";
	static inline constexpr rttr::string_view C_COMPONENT_DESTROY_ALL_FUNC_NAME = "destroy_all";
	static inline constexpr rttr::string_view C_COMPONENT_GET_COMPONENT_FUNC_NAME = "get";

	//- Base class for all components. See components below as examples on how to define new components.
	//------------------------------------------------------------------------------------------------------------------------
	struct icomponent
	{
		static std::string_view name() { static constexpr std::string_view C_NAME = "icomponent"; return C_NAME; };

		RTTR_ENABLE()
	};

} //- kokoro::ecs

namespace rttr
{
	namespace detail
	{
		template <typename U>
		static auto detect_custom_serialize(int) -> decltype((void)&U::custom_serialize, std::true_type{});

		template <typename>
		static std::false_type detect_custom_serialize(...);

		template <typename U>
		static auto detect_custom_deserialize(int) -> decltype((void)&U::custom_deserialize, std::true_type{});

		template <typename>
		static std::false_type detect_custom_deserialize(...);

		template <typename U>
		static auto detect_show_ui(int) -> decltype((void)&U::show_ui, std::true_type{});

		template <typename>
		static std::false_type detect_show_ui(...);

	} //- detail

	//- Utility class for RTTR component registration. Does register default serialize and deserialize functions.
	//------------------------------------------------------------------------------------------------------------------------
	template<class TComponent>
	class ccomponent final : public cregistrator<TComponent>
	{
	public:
		ccomponent(rttr::string_view name) :
			cregistrator<TComponent>(name)
		{
			register_common_component_functions();
		}

		ccomponent& custom_serialization()
		{
			if constexpr (decltype(detail::detect_custom_serialize<TComponent>(0))::value &&
				decltype(detail::detect_custom_deserialize<TComponent>(0))::value)
			{
				this->meth(kokoro::ecs::C_COMPONENT_CUSTOM_SERIALIZE_FUNC_NAME, &TComponent::custom_serialize)
					.meth(kokoro::ecs::C_COMPONENT_CUSTOM_DESERIALIZE_FUNC_NAME, &TComponent::custom_deserialize);
			}
			return *this;
		}

	private:
		//- Note: custom_serialize, custom_deserialize and show_ui are optional.
		void register_common_component_functions()
		{
			this->ctor()
				.template ctor<kokoro::cworld*>()
				.meth(kokoro::ecs::C_COMPONENT_NAME_FUNC_NAME,					&TComponent::name)
				.meth(kokoro::ecs::C_COMPONENT_SERIALIZE_FUNC_NAME,				&TComponent::serialize)
				.meth(kokoro::ecs::C_COMPONENT_DESERIALIZE_FUNC_NAME,			&TComponent::deserialize)
				.meth(kokoro::ecs::C_COMPONENT_DEFAULT_SERIALIZE_FUNC_NAME,		&TComponent::do_default_serialize)
				.meth(kokoro::ecs::C_COMPONENT_DEFAULT_DESERIALIZE_FUNC_NAME,	&TComponent::do_default_deserialize)
				.meth(kokoro::ecs::C_COMPONENT_SET_FUNC_NAME,					&TComponent::set)
				.meth(kokoro::ecs::C_COMPONENT_ADD_FUNC_NAME,					&TComponent::add)
				.meth(kokoro::ecs::C_COMPONENT_REMOVE_FUNC_NAME,				&TComponent::remove)
				.meth(kokoro::ecs::C_COMPONENT_ADD_SINGLETON_FUNC_NAME,			&TComponent::add_singleton)
				.meth(kokoro::ecs::C_COMPONENT_REMOVE_SINGLETON_FUNC_NAME,		&TComponent::remove_singleton)
				.meth(kokoro::ecs::C_COMPONENT_HAS_FUNC_NAME,					&TComponent::has)
				.meth(kokoro::ecs::C_COMPONENT_DESTROY_ALL_FUNC_NAME,			&TComponent::destroy_all)
				.meth(kokoro::ecs::C_COMPONENT_GET_COMPONENT_FUNC_NAME,			&TComponent::get)
				;

			if constexpr (decltype(detail::detect_show_ui<TComponent>(0))::value)
			{
				this->meth(kokoro::ecs::C_COMPONENT_SHOW_UI_FUNC_NAME, &TComponent::show_ui);
			}
		}
	};

} //- rttr
