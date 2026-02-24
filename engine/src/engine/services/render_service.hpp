#pragma once
#include <engine/iservice.hpp>
#include <bgfx.h>

namespace kokoro
{
	inline static constexpr uint16_t C_BACKBUFFER_PASS_ID	= 0;
	inline static constexpr uint16_t C_GEOMETRY_PASS_ID		= 1;
	inline static constexpr uint16_t C_POSTPROCESS_PASS_ID	= C_GEOMETRY_PASS_ID + 1;
	inline static constexpr uint16_t C_MERGE_PASS_ID		= 254;
	inline static constexpr uint16_t C_IMGUI_PASS_ID		= 255;

	//------------------------------------------------------------------------------------------------------------------------
	class crender_service final : public iservice
	{
	public:
		crender_service() = default;
		~crender_service() = default;

		bool		init() override final;
		void		post_init() override final;
		void		shutdown() override final;
		void		update(float dt) override final;

		void		begin_frame();
		void		end_frame();

	private:
	};

} //- kokoro