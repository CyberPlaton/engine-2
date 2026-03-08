#pragma once
#include <engine/iservice.hpp>
#include <engine/ilayer.hpp>
#include <unordered_map>
#include <typeinfo>
#include <memory>
#include <atomic>

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class cengine final
	{
	public:
		static cengine& instance();

		cengine() = default;
		~cengine() = default;

		bool		init();
		void		shutdown();
		void		run();
		void		exit();

		template<typename T, typename... ARGS>
		cengine&	new_service(ARGS... args);

		template<typename T, typename... ARGS>
		cengine&	new_layer(ARGS... args);

		template<typename T>
		T&			service() const;

	private:
		std::unordered_map<uint64_t, uint64_t> m_services_mapping;
		std::vector<std::unique_ptr<iservice>> m_services;
		std::vector<std::unique_ptr<ilayer>> m_layers;
		std::atomic_bool m_running = false;
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename T, typename... ARGS>
	cengine& cengine::new_service(ARGS... args)
	{
		const auto idx = m_services.size();
		m_services_mapping[typeid(T).hash_code()] = idx;
		m_services.emplace_back(std::move(
			std::make_unique<T>(std::forward<ARGS>(args)...)
		));
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename T, typename... ARGS>
	cengine& cengine::new_layer(ARGS... args)
	{
		m_layers.emplace_back(std::move(
			std::make_unique<T>(std::forward<ARGS>(args)...)
		));
		return* this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename T>
	T& cengine::service() const
	{
		return *reinterpret_cast<T*>(m_services[m_services_mapping.at(typeid(T).hash_code())].get());
	}

	cengine& instance();

} //- kokoro