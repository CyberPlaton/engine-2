#pragma once
#include <engine/iservice.hpp>
#include <taskflow.h>
#include <memory>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class cthread_service final : public iservice
	{
	public:
		cthread_service() = default;
		~cthread_service() = default;

		bool			init() override final;
		void			post_init() override final;
		void			shutdown() override final;
		void			update(float dt) override final;

		template<typename TCallback>
		decltype(auto)	async(const std::string& name, TCallback&& cb)
		{
			return m_executor->async(name, std::move(cb));
		}

	private:
		std::unique_ptr<tf::Executor> m_executor;
	};

} //- kokoro