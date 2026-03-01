#pragma once
#include <vector>
#include <string>
#include <rttr.h>
#include <nlohmann.h>

namespace kokoro
{
	//- Class representing serialized state of entities of a prefab or a world.
	//------------------------------------------------------------------------------------------------------------------------
	struct sscene final
	{
		struct scomponent;
		struct sentity;
		using components_t = std::vector<scomponent>;
		using entities_t = std::vector<sentity>;

		struct scomponent
		{
			std::string m_type_name;
			rttr::variant m_data;
			RTTR_ENABLE();
		};

		struct sentity
		{
			std::string m_name;
			std::string m_uuid;
			components_t m_comps;
			entities_t m_kids;
			RTTR_ENABLE();
		};

		std::string m_name;
		entities_t m_entities;
		RTTR_ENABLE();
	};

	namespace scene
	{
		static inline constexpr rttr::string_view C_ENTITIES_PROP = "entities";
		static inline constexpr rttr::string_view C_SYSTEMS_PROP = "systems";
		static inline constexpr rttr::string_view C_MODULES_PROP = "modules";
		static inline constexpr rttr::string_view C_COMPONENTS_PROP = "components";

		//- Function to convert a scene to a JSON string
		nlohmann::json	serialize(const sscene& scene);

		//- Function to extract a scene from JSON
		sscene			deserialize(const nlohmann::json& json);

	} //- scene

} //- kokoro
