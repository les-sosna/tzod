// Application.h

#pragma once

#include "core/SafePtr.h"

class NetworkInitHelper;

class AppBase : public RefCounted
{
public:
	AppBase();
	virtual ~AppBase();

	int Run(HINSTANCE hInst);

	void RegisterHandle(HANDLE h, Delegate<void()> callback);
	void UnregisterHandle(HANDLE h);

	ConsoleBuffer* GetConsole() const { return GetRawPtr(_console); }

	void InitNetwork();


	virtual bool Pre()  = 0;
	virtual void Idle() = 0;
	virtual void Post() = 0;

private:
	HINSTANCE _hinst;
	std::vector<HANDLE> _handles;
	std::vector<Delegate<void()> > _callbacks;
	SafePtr<ConsoleBuffer> _console;
	SafePtr<NetworkInitHelper> _netHelper;
};

// end of file
