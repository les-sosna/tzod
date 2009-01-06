// Application.cpp

#include "stdafx.h"
#include "Application.h"
#include "Console.h"

#include "network/init.h"


AppBase::AppBase()
  : _hinst(NULL)
  , _console(new ConsoleBuffer(128, 512, "log.txt"))
{
}

AppBase::~AppBase()
{
}

void AppBase::InitNetwork()
{
	if( !_netHelper )
	{
		_netHelper = WrapRawPtr(new NetworkInitHelper());
	}
}

int AppBase::Run(HINSTANCE hInst)
{
	_ASSERT(!_hinst);
	_hinst = hInst;
	if( Pre() )
	{
		for(;;)
		{
			MSG msg;
			while( PeekMessage(&msg, NULL, 0, 0, TRUE) )
			{
				if( WM_QUIT == msg.message )
				{
					Post();
					return msg.wParam;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			while( _handles.size() )
			{
				DWORD result = WaitForMultipleObjects(_handles.size(), &_handles[0], FALSE, 0);
				if( WAIT_TIMEOUT == result )
					break; // no objects in signaled state
				_ASSERT(result < WAIT_OBJECT_0 + _handles.size());

				// process signaled object
				INVOKE(_callbacks[result - WAIT_OBJECT_0]) ();
			}
			Idle();
		}
		_ASSERT(false);
	}
	Post();
	return -1;
}

void AppBase::RegisterHandle(HANDLE h, Delegate<void()> callback)
{
	_ASSERT(_handles.size() < MAXIMUM_WAIT_OBJECTS);
	_handles.push_back(h);
	_callbacks.push_back(callback);
	_ASSERT(_handles.size() == _callbacks.size());
}

void AppBase::UnregisterHandle(HANDLE h)
{
	std::vector<HANDLE>::iterator it = std::find(_handles.begin(), _handles.end(), h);
	_ASSERT(_handles.end() != it);
	_callbacks.erase(_callbacks.begin() + (it - _handles.begin()));
	_handles.erase(it);
}

// end of file
