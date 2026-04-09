#pragma once
#include <engine/math/vec2.hpp>
#include <engine/services/resource_manager_service.hpp>
#include <rttr.h>
#include <bgfx.h>
#include <cstdint>
#include <vector>
#include <optional>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	enum mesh_usage_type : uint8_t
	{
		mesh_usage_type_none = 0,
		mesh_usage_type_static,
		mesh_usage_type_dynamic,
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct smesh_snapshot final
	{
		std::vector<math::vec2_t> m_vertices;
		std::vector<math::vec2_t> m_uvs;
		std::vector<uint32_t> m_colors;
		std::vector<uint16_t> m_indices;
		mesh_usage_type m_type = mesh_usage_type_none;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct smesh
	{
		static std::optional<smesh>	load(const rttr::variant& snapshot);
		static void					unload(smesh& mesh);

		union
		{
			bgfx::VertexBufferHandle m_vbh_static = BGFX_INVALID_HANDLE;
			bgfx::DynamicVertexBufferHandle m_vbh_dynamic;
		};
		union
		{
			bgfx::IndexBufferHandle m_ibh_static = BGFX_INVALID_HANDLE;
			bgfx::DynamicIndexBufferHandle m_ibh_dynamic;
		};

		unsigned m_vertex_count	= 0;
		unsigned m_index_count	= 0;
		mesh_usage_type m_type	= mesh_usage_type_none;
	};

} //- kokoro