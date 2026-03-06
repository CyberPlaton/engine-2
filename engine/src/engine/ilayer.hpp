#pragma once

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class ilayer
	{
	public:
		virtual ~ilayer() = default;

		//- Required initialization function
		virtual bool	init() = 0;

		//- Optional initialization function, called after normal init and before hot start
		//- of application, when the render and window are ready
		virtual void	post_init() {};

		virtual void	shutdown() {};
		virtual void	update(float) {};
	};

} //- kokoro