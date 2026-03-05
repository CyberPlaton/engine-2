#pragma once
#include <cstdint>
#include <vector>
#include <engine/math/vec3.hpp>
#include <engine/math/vec4.hpp>
#include <engine/iresource_manager_service.hpp>
#include <rttr.h>
#include <bgfx.h>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	struct smesh_snapshot final
	{
		math::vec3_t m_v1 = { -0.5f, 0.5f, 0.0f };
		math::vec3_t m_v2 = { 0.5f, 0.5f, 0.0f };
		math::vec3_t m_v3 = { -0.5f, -0.5f, 0.0f };
		math::vec3_t m_v4 = { 0.5f, -0.5f, 0.0f };
		math::vec4_t m_source = { 0.0f, 0.0f, 1.0f, 1.0f };
		RTTR_ENABLE();
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct smesh final
	{
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

		std::vector<sgroup> m_groups;
		bgfx::VertexLayout m_layout;
	};

	//------------------------------------------------------------------------------------------------------------------------
	class cmesh_resource_manager_service final : public iresource_manager_service<smesh, smesh_snapshot>
	{
	public:
		cmesh_resource_manager_service() = default;
		~cmesh_resource_manager_service() = default;

	protected:
		smesh			do_instantiate(const smesh_snapshot* snaps) override final;
		void			do_destroy(smesh* inst) override final;
	};

} //- kokoro