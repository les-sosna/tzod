// Application.h

#pragma once

#include "core/SafePtr.h"

class NetworkInitHelper;

class AppBase : public RefCounted
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
	SafePtr<NetworkInitHelper> _netHelper;
};

// end of file
