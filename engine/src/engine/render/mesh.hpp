#pragma once
#include <engine/render/vertices.hpp>
#include <engine/math/vec3.hpp>
#include <rttr.h>
#include <cstdint>
#include <vector>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class cmesh final
	{
	public:
		struct ssnapshot
		{
			math::vec3_t m_v1 = { -0.5f, 0.5f, 0.0f };
			math::vec3_t m_v2 = { 0.5f, 0.5f, 0.0f };
			math::vec3_t m_v3 = { -0.5f, -0.5f, 0.0f };
			math::vec3_t m_v4 = { 0.5f, -0.5f, 0.0f };
			RTTR_ENABLE();
		};

		struct sgroup
		{
			struct sprimitive
			{
				unsigned m_start_index = std::numeric_limits<unsigned>().max();
				unsigned m_start_vertex = std::numeric_limits<unsigned>().max();
				unsigned m_index_count = std::numeric_limits<unsigned>().max();
				unsigned m_vertex_count = std::numeric_limits<unsigned>().max();
			};

			std::vector<sprimitive> m_primitives;
			bgfx::VertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
			bgfx::IndexBufferHandle m_ibh = BGFX_INVALID_HANDLE;
			unsigned m_vertex_count = 0;
			uint8_t* m_vertices = nullptr;
			uint16_t* m_indices = nullptr;
			unsigned m_index_count = 0;
		};

	private:
		std::vector<sgroup> m_groups;
		bgfx::VertexLayout m_layout;
	};

} //- kokoro