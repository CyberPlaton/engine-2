#pragma once
#include <core/uuid.hpp>
#include <engine/scene.hpp>
#include <optional>

namespace kokoro
{
	namespace world
	{
		struct sworld;

		namespace prefab
		{
			//- Additional configuration for loading a prefab.
			//------------------------------------------------------------------------------------------------------------------------
			struct sconfig final
			{
			};

			//- JSON conversion functions that ought to be used when writing or reading a prefab is required. Note that anything else
			//- might not produce valid results.
			nlohmann::json					serialize(const sscene& prefab);
			sscene							deserialize(const nlohmann::json& json);

			//- Find the filepath of a runtime prefab given its unique identifier, a uuid is used because from one prefab resource
			//- we can create many prefab entities.
			std::string						filepath(const core::cuuid& uuid);

			//- Create a runtime prefab for world either from file or from loaded raw data, note that prefabs loaded from raw data
			//- do not return a valid filepath. Moreover, the function can be consider 'instantiate' of a prefab into the current world.
			[[nodiscard]] core::cuuid		create(sworld* w, std::string_view filepath, std::optional<sconfig> cfg = std::nullopt);
			[[nodiscard]] core::cuuid		create(sworld* w, const sscene& data, std::optional<sconfig> cfg = std::nullopt);

			//- Release runtime prefab from world and from internal storage. Does nothing if no prefab exists.
			void							destroy(sworld* w, const core::cuuid& uuid);

			//- Release all runtime prefabs at once.
			void							cleanup(sworld* w);

		} //- prefab

	} //- world

} //- kokoro
