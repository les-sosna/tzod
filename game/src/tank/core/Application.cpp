// Application.cpp

#include "stdafx.h"
#include "Application.h"

#include "network/init.h"


AppBase::AppBase()
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

int AppBase::Run()
{
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
				assert(result < WAIT_OBJECT_0 + _handles.size());

				// process signaled object
				INVOKE(_callbacks[result - WAIT_OBJECT_0]) ();
			}
			Idle();
		}
		assert(false);
	}
	Post();
	return -1;
}

void AppBase::RegisterHandle(HANDLE h, Delegate<void()> callback)
{
	assert(_handles.size() < MAXIMUM_WAIT_OBJECTS);
	_handles.push_back(h);
	_callbacks.push_back(callback);
	assert(_handles.size() == _callbacks.size());
}

void AppBase::UnregisterHandle(HANDLE h)
{
	std::vector<HANDLE>::iterator it = std::find(_handles.begin(), _handles.end(), h);
	assert(_handles.end() != it);
	_callbacks.erase(_callbacks.begin() + (it - _handles.begin()));
	_handles.erase(it);
}

// end of file
