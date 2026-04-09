#include <engine/render/mesh.hpp>
#include <engine/render/vertices.hpp>
#include <engine/services/virtual_filesystem_service.hpp>
#include <engine/services/log_service.hpp>
#include <core/mutex.hpp>
#include <core/io.hpp>
#include <engine.hpp>
#include <fmt.h>
#include <dlmalloc.h>
#include <string_view>

namespace kokoro
{
	namespace
	{
		//------------------------------------------------------------------------------------------------------------------------
		bool validate(const smesh_snapshot& snaps)
		{
			if (snaps.m_vertices.empty() || snaps.m_indices.empty())
			{
				return false;
			}
			if (!snaps.m_uvs.empty() &&
				snaps.m_vertices.size() != snaps.m_uvs.size())
			{
				return false;
			}
			if (!snaps.m_colors.empty() &&
				snaps.m_vertices.size() != snaps.m_colors.size())
			{
				return false;
			}
			return true;
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	std::optional<smesh> smesh::load(const rttr::variant& snapshot)
	{
		if (!snapshot.is_valid())
		{
			log::err("smesh::load - received invalid snapshot variant");
			return std::nullopt;
		}

		const auto& snaps = snapshot.get_value<smesh_snapshot>();
		const auto& layout = spos_color_uv_vertex::get().layout();
		smesh mesh;

		if (!validate(snaps))
		{
			log::err("smesh::load - mesh snapshot is invalid");
			return std::nullopt;
		}

		const auto buffer_size = snaps.m_vertices.size() * layout.getStride();
		auto* buffer = KOKORO_MALLOC(buffer_size);
		auto* vertices = reinterpret_cast<spos_color_uv_vertex*>(buffer);

		for (auto i = 0; i < snaps.m_vertices.size(); ++i)
		{
			const auto& position = snaps.m_vertices[i];
			vertices[i] = { position.x, position.y, 0.0f, 0xffffffff, 0.0f, 0.0f };
		}

		for (auto i = 0; i < snaps.m_uvs.size(); ++i)
		{
			const auto& uv = snaps.m_uvs[i];
			vertices[i].m_u = uv.x;
			vertices[i].m_v = uv.y;
		}

		for (auto i = 0; i < snaps.m_colors.size(); ++i)
		{
			const auto& color = snaps.m_colors[i];
			vertices[i].m_abgr = color;
		}

		const auto* vertex_memory = bgfx::copy(buffer, buffer_size);
		const auto* index_memory = bgfx::copy(snaps.m_indices.data(), snaps.m_indices.size() * sizeof(uint16_t));
		KOKORO_FREE(buffer);

		switch (snaps.m_type)
		{
		case mesh_usage_type_dynamic:
		{
			mesh.m_vbh_dynamic = bgfx::createDynamicVertexBuffer(vertex_memory, layout);
			mesh.m_ibh_dynamic = bgfx::createDynamicIndexBuffer(index_memory);

			if (!bgfx::isValid(mesh.m_vbh_dynamic) || !bgfx::isValid(mesh.m_ibh_dynamic))
			{
				log::err("smesh::load - failed creating buffers for dynamic mesh");
				return std::nullopt;
			}
			break;
		}
		case mesh_usage_type_static:
		{
			mesh.m_vbh_static = bgfx::createVertexBuffer(vertex_memory, layout);
			mesh.m_ibh_static = bgfx::createIndexBuffer(index_memory);

			if (!bgfx::isValid(mesh.m_vbh_static) || !bgfx::isValid(mesh.m_ibh_static))
			{
				log::err("smesh::load - failed creating buffers for static mesh");
				return std::nullopt;
			}
			break;
		}
		}

		mesh.m_vertex_count = snaps.m_vertices.size();
		mesh.m_index_count = snaps.m_indices.size();
		mesh.m_type = snaps.m_type;

		return std::move(mesh);
	}

	//------------------------------------------------------------------------------------------------------------------------
	void smesh::unload(smesh& mesh)
	{
		switch (mesh.m_type)
		{
		case mesh_usage_type_static:
		{
			if (bgfx::isValid(mesh.m_vbh_static))
			{
				bgfx::destroy(mesh.m_vbh_static);
			}
			if (bgfx::isValid(mesh.m_ibh_static))
			{
				bgfx::destroy(mesh.m_ibh_static);
			}
			break;
		}
		case mesh_usage_type_dynamic:
		{
			if (bgfx::isValid(mesh.m_vbh_dynamic))
			{
				bgfx::destroy(mesh.m_vbh_dynamic);
			}
			if (bgfx::isValid(mesh.m_ibh_dynamic))
			{
				bgfx::destroy(mesh.m_ibh_dynamic);
			}
			break;
		}
		}
	}

} //- kokoro