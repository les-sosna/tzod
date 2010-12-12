// Application.h

#pragma once

class NetworkInitHelper;

class AppBase
{
public:
	AppBase();
	virtual ~AppBase();

	int Run();

	void RegisterHandle(HANDLE h, Delegate<void()> callback);
	void UnregisterHandle(HANDLE h);

	void InitNetwork();


	virtual bool Pre()  = 0;
	virtual void Idle() = 0;
	virtual void Post() = 0;

private:
	std::vector<HANDLE> _handles;
	std::vector<Delegate<void()> > _callbacks;
	std::auto_ptr<NetworkInitHelper> _netHelper;
};

// end of file
