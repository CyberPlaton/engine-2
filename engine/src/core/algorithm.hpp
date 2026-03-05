#pragma once

namespace kokoro::core
{
	//------------------------------------------------------------------------------------------------------------------------
	template<typename TIterator, typename TCallback>
	TIterator find_if(TIterator begin, TIterator end, TCallback&& callback)
	{
		for (auto it = begin; it < end; ++it)
		{
			if (callback(*it))
			{
				return it;
			}
		}
		return nullptr;
	}

	//------------------------------------------------------------------------------------------------------------------------
	template<typename TContainer, typename TIterator>
	void erase(TContainer& c, TIterator it)
	{
		c.erase(it);
	}


} //- kokoro::core