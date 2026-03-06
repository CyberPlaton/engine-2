#include <engine/render/mesh.hpp>
#include <engine/render/vertices.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/mutex.hpp>
#include <core/io.hpp>
#include <engine.hpp>
#include <fmt.h>
#include <dlmalloc.h>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	bool cmesh_resource_manager_service::init()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	smesh cmesh_resource_manager_service::do_instantiate(const smesh_snapshot* snaps)
	{
		if (!bgfx::isValid(spos_color_uv_vertex::S_HANDLE))
		{
			spos_color_uv_vertex::init();
		}

		smesh mesh;
		mesh.m_layout = spos_color_uv_vertex::S_LAYOUT;

		const auto u_start = snaps->m_source.x;
		const auto u_end = snaps->m_source.x + snaps->m_source.z;
		const auto v_start = snaps->m_source.y;
		const auto v_end = snaps->m_source.y + snaps->m_source.w;

		spos_color_uv_vertex vertices[] =
		{
			{ snaps->m_v1.x, snaps->m_v1.y, snaps->m_v1.z, 0xffffffff, u_start, v_end },		//- Bottom-left
			{ snaps->m_v2.x, snaps->m_v2.y, snaps->m_v2.z, 0xffffffff, u_end, v_end },		//- Bottom-right
			{ snaps->m_v3.x, snaps->m_v3.y, snaps->m_v3.z, 0xffffffff, u_start, v_start },	//- Top-left
			{ snaps->m_v4.x, snaps->m_v4.y, snaps->m_v4.z, 0xffffffff, u_end, v_start }		//- Top-right
		};

		uint16_t indices[] = {
			0, 1, 2, //- First triangle
			1, 3, 2  //- Second triangle
		};

		const auto sprite_vertex_size = sizeof(spos_color_uv_vertex);
		const auto vertices_size = sizeof(vertices);
		const auto indices_size = sizeof(indices);

		//- Create vertex buffer
		const bgfx::Memory* vb_mem = bgfx::copy(vertices, vertices_size);
		bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(vb_mem, mesh.m_layout);

		//- Create index buffer
		const bgfx::Memory* ib_mem = bgfx::copy(indices, sizeof(indices));
		bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(ib_mem);

		//- Create a primitive
		smesh::sgroup::sprimitive prim;
		prim.m_start_index = 0;
		prim.m_start_vertex = 0;
		prim.m_index_count = indices_size / sizeof(uint16_t);
		prim.m_vertex_count = vertices_size / sprite_vertex_size;

		//- Create a group
		smesh::sgroup group;
		group.m_vbh = vbh;
		group.m_ibh = ibh;

		group.m_vertices = (uint8_t*)KOKORO_MALLOC(vb_mem->size);
		group.m_vertex_count = vb_mem->size;
		bx::memCopy(group.m_vertices, vb_mem->data, group.m_vertex_count);

		group.m_indices = (uint16_t*)KOKORO_MALLOC(ib_mem->size);
		group.m_index_count = ib_mem->size;
		bx::memCopy(group.m_indices, ib_mem->data, group.m_index_count);

		group.m_primitives.push_back(prim);

		return mesh;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cmesh_resource_manager_service::do_destroy(smesh* inst)
	{
		for (auto& group : inst->m_groups)
		{
			if (bgfx::isValid(group.m_vbh))
			{
				bgfx::destroy(group.m_vbh);
			}
			if (bgfx::isValid(group.m_ibh))
			{
				bgfx::destroy(group.m_ibh);
			}
			KOKORO_FREE(group.m_vertices);
			KOKORO_FREE(group.m_indices);
		}
	}

} //- kokoro