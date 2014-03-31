// Application.h

#pragma once

class AppBase
{
public:
	AppBase();
	virtual ~AppBase();

	int Run();

	virtual bool Pre()  = 0;
	virtual void Idle() = 0;
	virtual void Post() = 0;
};

// end of file
