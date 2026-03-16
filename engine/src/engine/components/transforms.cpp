#include <engine/components/transforms.hpp>

namespace kokoro::world::component
{
	using matrix_serialize_type = std::array<float, 16u>;

	//------------------------------------------------------------------------------------------------------------------------
	bool slocal_transform::show_ui(rttr::variant& comp)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	bool sworld_transform::show_ui(rttr::variant& comp)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void sworld_transform::custom_serialize(const rttr::variant& var, nlohmann::json& json)
	{
		if (const auto type = var.get_type(); type.is_valid() && type == rttr::type::get<sworld_transform>())
		{
			const auto& t = var.get_value<sworld_transform>();
			const auto type_name = type.get_name().data();

			json = nlohmann::json::object();
			json[core::C_OBJECT_TYPE_NAME] = type_name;

			//- Serialize transform data
			nlohmann::json j;
			j = nlohmann::json::object();

			//- Copy matrix to array and serialize as sequence of 16 floats
			{
				matrix_serialize_type matrix;
				std::memcpy(&matrix, &t.m_matrix.value, sizeof(float) * 16);
				j["matrix"] = core::to_json_object(matrix);
			}

			//- AABB
			{
				auto& aabb = j["aabb"];
				auto& oobb = j["oobb"];

				aabb["x"] = t.m_aabb.aabb().lowerBound.x;
				aabb["y"] = t.m_aabb.aabb().lowerBound.y;
				aabb["z"] = t.m_aabb.aabb().upperBound.x;
				aabb["w"] = t.m_aabb.aabb().upperBound.y;
			}

			json[type_name] = j;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	void sworld_transform::custom_deserialize(rttr::variant& var, nlohmann::json& json)
	{
		if (json.contains(core::C_OBJECT_TYPE_NAME))
		{
			const auto type_name = json[core::C_OBJECT_TYPE_NAME].get<std::string>();
			if (const auto type = rttr::type::get_by_name(type_name); type.is_valid() && type == rttr::type::get<sworld_transform>())
			{
				auto& j = json[type_name];
				auto& t = var.get_value<sworld_transform>();

				//- Load matrix and values into transform
				{
					if (auto var = core::from_json_object(rttr::type::get<matrix_serialize_type>(), j["matrix"]); var.is_valid())
					{
						auto matrix = var.get_value<matrix_serialize_type>();
						std::memcpy(&t.m_matrix.value, &matrix, sizeof(float) * 16);
					}
				}

				//- AABB
				{
					auto& aabb = j["aabb"];
					auto& oobb = j["oobb"];

					t.m_aabb.aabb().lowerBound.x = aabb["x"].get<float>();
					t.m_aabb.aabb().lowerBound.y = aabb["y"].get<float>();
					t.m_aabb.aabb().upperBound.x = aabb["z"].get<float>();
					t.m_aabb.aabb().upperBound.y = aabb["w"].get<float>();
				}
			}
		}
	}

} //- kokoro::world::component

RTTR_REGISTRATION
{
	using namespace kokoro::world::component;

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<slocal_transform>("slocal_transform")
		.prop("m_position", &slocal_transform::m_position)
		.prop("m_scale", &slocal_transform::m_scale)
		.prop("m_origin", &slocal_transform::m_origin)
		.prop("m_rotation", &slocal_transform::m_rotation);

	//------------------------------------------------------------------------------------------------------------------------
	rttr::ccomponent<sworld_transform>("sworld_transform")
		.custom_serialization()
		.prop("m_matrix", &sworld_transform::m_matrix)
		.prop("m_aabb", &sworld_transform::m_aabb);
}