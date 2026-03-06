#pragma once
#include <bgfx.h>
#include <bimg.h>
#include <string>
#include <engine/iresource_manager_service.hpp>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	struct stexture
	{
		using format = bgfx::TextureFormat::Enum;

		bimg::ImageContainer m_image;
		bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;
	};

	//------------------------------------------------------------------------------------------------------------------------
	struct stexture_snapshot final
	{
		std::string m_texture;
	};

	//------------------------------------------------------------------------------------------------------------------------
	class ctexture_resource_manager_service final : public iresource_manager_service<stexture, stexture_snapshot, false>
	{
	public:
		ctexture_resource_manager_service() = default;
		~ctexture_resource_manager_service() = default;

		bool			init() override final;

	protected:
		stexture		do_instantiate(const stexture_snapshot* snaps) override final;
		void			do_destroy(stexture* inst) override final;
	};

} //- kokoro