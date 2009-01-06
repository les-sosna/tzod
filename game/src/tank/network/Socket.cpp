// socket.cpp

#include "stdafx.h"
#include "socket.h"

#include "ui/interface.h"

#include "core/debug.h"
#include "core/Console.h"
#include "core/Application.h"

///////////////////////////////////////////////////////////////////////////////

Socket::Socket(void)
{
	_event  = WSA_INVALID_EVENT;
	_socket = INVALID_SOCKET;
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
		WSAEventSelect(_socket, _event, 0); // cancel event association
		WSACloseEvent(_event);
		_event = WSA_INVALID_EVENT;
	}

	u_long ulParam = 0;
	ioctlsocket(_socket, FIONBIO, &ulParam);  // return back to blocking mode

	int result = closesocket(_socket);
	_socket = INVALID_SOCKET;

	return result;
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
