// socket.cpp

#include "stdafx.h"
#include "socket.h"

#include "ui/interface.h"

#include "core/debug.h"
#include "core/Application.h"

///////////////////////////////////////////////////////////////////////////////

Socket::Socket(SOCKET s)
  : _event(WSA_INVALID_EVENT)
  , _socket(INVALID_SOCKET)
  , _hasCallback(false)
{
	if( INVALID_SOCKET != s )
	{
		Attach(s);
	}
}

Socket::~Socket()
{
	assert(WSA_INVALID_EVENT == _event);
	assert(INVALID_SOCKET == _socket);
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
	assert(INVALID_SOCKET != _socket);

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
		TRACE("peer: return back to blocking mode - ok");
	}
	else
	{
		TRACE("peer: return back to blocking mode - %d", WSAGetLastError());
	}

	if( !shutdown(_socket, SD_SEND) )
	{
		TRACE("peer: socket shutdown - ok");
	}
	else
	{
		TRACE("peer: socket shutdown - %d", WSAGetLastError());
	}

	if( !closesocket(_socket) )
	{
		TRACE("peer: close socket - ok");
	}
	else
	{
		TRACE("peer: close socket - %d", WSAGetLastError());
	}

	_socket = INVALID_SOCKET;

	return 0;
}

void Socket::SetCallback(Delegate<void()> callback)
{
	assert(!_hasCallback);
	assert(INVALID_SOCKET != _socket);
	assert(WSA_INVALID_EVENT != _event);
	g_app->RegisterHandle(_event, callback);
	_hasCallback = true;
}

void Socket::Attach(SOCKET s)
{
	assert(NULL == _event);
	assert(INVALID_SOCKET == _socket);
	assert(INVALID_SOCKET != s);
	_socket = s;
//	BOOL on = TRUE;
//	if( setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*) &on, sizeof(BOOL)) )
//	{
//		TRACE("WARNING: setting TCP_NODELAY failed!");
//	}
//	int bufsize = 0;
//	if( setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*) &bufsize, sizeof(int)) )
//	{
//		TRACE("WARNING: setting SO_SNDBUF failed! %d", WSAGetLastError());
//	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file
