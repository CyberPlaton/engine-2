#pragma once
#include <flecs.h>
#include <memory>
#include <vector>

namespace kokoro::world
{
	class ichange_tracker;
	using change_tracker_t = std::weak_ptr<ichange_tracker>;

	//------------------------------------------------------------------------------------------------------------------------
	class ichange_tracker
	{
	public:
		virtual ~ichange_tracker() = default;

		virtual void						tick() = 0;
		virtual bool						changed() = 0;
		virtual std::vector<flecs::entity>	entities() const = 0;
	};

	//- Class responsible for tracking per-frame data changes of an ecs component archetype, caches results and allows
	//- multiple systems in different contexts and threads to access and process the results.
	//- If you are sure that you are the only one who will query state changes of an archetype, then feel free to use flecs::query
	//- directly. Otherwise use this class through worlds query manager to avoid bugs and unexpected behavior.
	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps>
	class cchange_tracker final : public ichange_tracker
	{
	public:
		cchange_tracker(flecs::world& w);
		virtual ~cchange_tracker();

		void			tick() override final;
		bool			changed() override final;
		entity_set_t	entities() const override final;

	private:
		std::vector<flecs::entity> m_entities;
		flecs::query<TComps...> m_tracker;
	};

	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps>
	void cchange_tracker<TComps...>::tick()
	{
		//- Clear previous changes
		m_entities.clear();

		//- React to changes of this frame
		if (m_tracker.changed())
		{
			m_tracker.run([this](flecs::iter& it)
				{
					while (it.next())
					{
						for (auto i : it)
						{
							auto e = it.entity(i);
							m_entities.emplace_back(e);
						}
					}
				});
		}
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps>
	std::vector<flecs::entity> cchange_tracker<TComps...>::entities() const
	{
		return m_entities;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps>
	bool cchange_tracker<TComps...>::changed()
	{
		return !m_entities.empty();
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps>
	cchange_tracker<TComps...>::~cchange_tracker()
	{
		m_tracker.destruct();
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename... TComps>
	cchange_tracker<TComps...>::cchange_tracker(flecs::world& w)
	{
		m_tracker = w
			.query_builder<TComps...>()
			.cached()
			.build();
	}
} //- kokoro::world