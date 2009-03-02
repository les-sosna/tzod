// socket.cpp

#include "stdafx.h"
#include "socket.h"

#include "ui/interface.h"

#include "core/debug.h"
#include "core/Console.h"
#include "core/Application.h"

///////////////////////////////////////////////////////////////////////////////

Socket::Socket(void)
  : _event(WSA_INVALID_EVENT)
  , _socket(INVALID_SOCKET)
  , _hasCallback(false)
{
}

Socket::~Socket(void)
{
	_ASSERT(WSA_INVALID_EVENT == _event);
	_ASSERT(INVALID_SOCKET == _socket);
}

int Socket::SetEvents(long lNetworkEvents)
{
	if( WSA_INVALID_EVENT == _event )
		_event = WSACreateEvent();
	if( WSA_INVALID_EVENT == _event )
		return SOCKET_ERROR;

	return WSAEventSelect(_socket, _event, lNetworkEvents);
}

int Socket::EnumNetworkEvents(LPWSANETWORKEVENTS lpNetworkEvents)
{
	if( WSAEnumNetworkEvents(_socket, _event, lpNetworkEvents) )
		return Error;
	return OK;
}

int Socket::Close()
{
	_ASSERT(INVALID_SOCKET != _socket);

	if( WSA_INVALID_EVENT != _event )
	{
		if( _hasCallback )
		{
			g_app->UnregisterHandle(_event);
			_hasCallback = false;
		}
		WSAEventSelect(_socket, _event, 0); // cancel event association
		WSACloseEvent(_event);
		_event = WSA_INVALID_EVENT;
	}

	u_long ulParam = 0;
	if( !ioctlsocket(_socket, FIONBIO, &ulParam) )  // return back to blocking mode
	{
		TRACE("peer: return back to blocking mode - ok\n");
	}
	else
	{
		TRACE("peer: return back to blocking mode - %d\n", WSAGetLastError());
	}

	if( !shutdown(_socket, SD_SEND) )
	{
		TRACE("peer: socket shutdown - ok\n");
	}
	else
	{
		TRACE("peer: socket shutdown - %d\n", WSAGetLastError());
	}

	if( !closesocket(_socket) )
	{
		TRACE("peer: close socket - ok\n");
	}
	else
	{
		TRACE("peer: close socket - %d\n", WSAGetLastError());
	}

	_socket = INVALID_SOCKET;

	return 0;
}

void Socket::SetCallback(Delegate<void()> callback)
{
	_ASSERT(!_hasCallback);
	_ASSERT(INVALID_SOCKET != _socket);
	_ASSERT(WSA_INVALID_EVENT != _event);
	g_app->RegisterHandle(_event, callback);
	_hasCallback = true;
}

SOCKET Socket::operator = (SOCKET s)
{
	_ASSERT(NULL == _event);
	_socket = s;
	BOOL on = TRUE;
	if( setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*) &on, sizeof(BOOL)) )
	{
		TRACE("WARNING: setting TCP_NODELAY failed!\n");
	}
	return _socket;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
