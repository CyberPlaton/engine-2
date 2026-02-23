#pragma once

namespace kokoro
{
	//------------------------------------------------------------------------------------------------------------------------
	class iservice
	{
	public:
		virtual ~iservice() = default;

		//- Required initialization function
		virtual bool	init() = 0;

		//- Optional initialization function, called after normal init and before hot start
		//- of application, when the render and window are ready
		virtual void	post_init() {};

		virtual void	shutdown() {};
		virtual void	update(float) {};
	};

} //- kokoro