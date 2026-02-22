#pragma once
#include <engine/iservice.hpp>
#include <unordered_map>
#include <typeinfo>
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

		template<typename T>
		T&			service() const;

	private:
		std::unordered_map<uint64_t, iservice*> m_services;
		std::atomic_bool m_running = false;
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename T, typename... ARGS>
	cengine& cengine::new_service(ARGS... args)
	{
		m_services[typeid(T).hash_code()] = new T(std::forward<ARGS>(args)...);
		return *this;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename T>
	T& cengine::service() const
	{
		return *reinterpret_cast<T*>(m_services.at(typeid(T).hash_code()));
	}

	cengine& instance();

} //- kokoro