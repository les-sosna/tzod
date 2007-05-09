// socket.cpp

#include "stdafx.h"
#include "socket.h"

#include "ui/interface.h"

#include "core/debug.h"

///////////////////////////////////////////////////////////////////////////////

Socket::Socket(void)
{
	_hEvent  = WSA_INVALID_EVENT;
	_hSocket = INVALID_SOCKET;
	ZeroMemory(&_ne, sizeof(WSANETWORKEVENTS));
}

Socket::~Socket(void)
{
	_ASSERT(WSA_INVALID_EVENT == _hEvent);
	_ASSERT(INVALID_SOCKET == _hSocket);
}

int Socket::SetEvents(long lNetworkEvents)
{
	if( WSA_INVALID_EVENT == _hEvent )
		_hEvent = WSACreateEvent();
	if( WSA_INVALID_EVENT == _hEvent )
		return SOCKET_ERROR;

	WSAResetEvent(_hEvent);
	return WSAEventSelect(_hSocket, _hEvent, lNetworkEvents);
}

int Socket::Recv(HANDLE hAbortEvent, void *buf, int len)
{
	_ASSERT(len > 0);
	_ASSERT(buf);

	if( SetEvents(FD_READ) )
		return Error;

	int recieved = 0;
	while( recieved < len )
	{
		// попытка получения данных
		int n = recv(_hSocket, (char *) buf + recieved, len - recieved, 0);
		if( SOCKET_ERROR == n || 0 == n )
		{
			// ожидание реакции сети
			int result = Wait(hAbortEvent);
			if( result ) return result;

			// проверка события
			if( !CheckEvent(FD_READ_BIT) )
				return Error;

			// получение данных
			n = recv(_hSocket, (char *) buf + recieved, len - recieved, 0);
			if( SOCKET_ERROR == n || 0 == n )
				return Error;
		}
		recieved += n;
	}

	return OK;
}

int Socket::Recv(const HANDLE *lphAbortEvents, size_t count, void *buf, int buflen)
{
	_ASSERT(lphAbortEvents && count);
	_ASSERT(buf && buflen > 0);

	if( SetEvents(FD_READ | FD_CLOSE) )
		return Error;

	int recieved = 0;
	while( recieved < buflen )
	{
		// попытка получения данных
		int n = recv(_hSocket, (char *) buf + recieved, buflen - recieved, 0);
		if( SOCKET_ERROR == n || 0 == n )
		{
			// ожидание реакции сети
			int result = Wait(lphAbortEvents, count);
			if( result ) return result;

			// проверка события
			if( !CheckEvent(FD_READ_BIT) )
			{
				if( CheckEvent(FD_CLOSE_BIT) )	Close();
				return Error;
			}

			// получение данных
			n = recv(_hSocket, (char *) buf + recieved, buflen - recieved, 0);
			if( SOCKET_ERROR == n || 0 == n )
				return Error;
		}
		recieved += n;
	}

	return OK;
}

int Socket::Send(HANDLE hAbortEvent, const void *buf, int len)
{
	_ASSERT(len > 0);
	_ASSERT(buf);

	if( SetEvents(FD_WRITE) )
		return Error;

	int sent = 0;
	while( sent < len )
	{
		// попытка отправки данных
		int n = send(_hSocket, (const char *) buf + sent, len - sent, 0);
		if( SOCKET_ERROR == n || 0 == n )
		{
			// ожидание реакции сети
			int result = Wait(hAbortEvent);
			if( result )
			{
				LOGOUT_1("wait error\n");
				return result;
			}

			// проверка события
			if( !CheckEvent(FD_WRITE_BIT) )
			{
				LOGOUT_1("check error\n");
				return Error;
			}

			// отправка данных
			n = send(_hSocket, (const char *) buf + sent, len - sent, 0);
			if( SOCKET_ERROR == n || 0 == n )
			{
				LOGOUT_1("send error\n");
				return Error;
			}
		}
		sent += n;
	}

	return OK;
}

int Socket::Send_accum(HANDLE hAbortEvent)
{
	if( _accum.empty() ) return OK;
	int result = Send(hAbortEvent, &(*_accum.begin()), _accum.size());
	_accum.clear();
    return result;
}

void Socket::Accumulate(const void *buf, int len)
{
	_accum.insert(_accum.end(), (char *) buf, (char *) buf + len);
}

int Socket::Wait()
{
	DWORD result = WSAWaitForMultipleEvents(1, &_hEvent, FALSE, WSA_INFINITE, FALSE);
	if( WSA_WAIT_EVENT_0 != result )
		return Error;
	ZeroMemory(&_ne, sizeof(WSANETWORKEVENTS));
	if( WSAEnumNetworkEvents(_hSocket, _hEvent, &_ne) )
		return Error;
	return OK;
}

int Socket::Wait(HANDLE hAbortEvent)
{
	HANDLE events[2] = {_hEvent, hAbortEvent};
	DWORD result = WSAWaitForMultipleEvents(2, events, FALSE, WSA_INFINITE, FALSE);
	if( WSA_WAIT_EVENT_0+1 == result )
		return Aborted;
	if( WSA_WAIT_EVENT_0 != result )
		return Error;
	ZeroMemory(&_ne, sizeof(WSANETWORKEVENTS));
	if( WSAEnumNetworkEvents(_hSocket, _hEvent, &_ne) )
		return Error;
	return OK;
}

int Socket::Wait(const HANDLE *lphAbortEvents, size_t count)
{
	std::vector<HANDLE> handles (lphAbortEvents, lphAbortEvents + count);
	handles.push_back(_hEvent);
	DWORD result = WaitForMultipleObjects(count+1, &handles.front(), FALSE, INFINITE);
	if( result - WAIT_OBJECT_0 < count )
		return Aborted + result - WAIT_OBJECT_0;
	if( result - WAIT_OBJECT_0 != count )
		return Error;
	ZeroMemory(&_ne, sizeof(WSANETWORKEVENTS));
	if( WSAEnumNetworkEvents(_hSocket, _hEvent, &_ne) )
		return Error;
	return OK;
}

int Socket::Close()
{
	_ASSERT(INVALID_SOCKET != _hSocket);

	int result = closesocket(_hSocket);

	if( WSA_INVALID_EVENT != _hEvent )
		WSACloseEvent(_hEvent);

	_hSocket = INVALID_SOCKET;
	_hEvent  = WSA_INVALID_EVENT;

	return result;
}

bool Socket::CheckEvent(int bit)
{
	if( 0 == (_ne.lNetworkEvents & (1 << bit)) || 0 != _ne.iErrorCode[bit] )
	{
		LOGOUT_1("CheckEvent failed: ");
		LOGOUT_2("lNetworkEvents = %d; ", _ne.lNetworkEvents);
		LOGOUT_3("iErrorCode[bit=%d] = %d\n", bit, _ne.iErrorCode[bit]);
		return false;
	}
	return true;
}

SOCKET Socket::operator = (SOCKET s)
{
	_ASSERT(NULL == _hEvent);
	_hSocket = s;
	BOOL on = TRUE;
	if( setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*) &on, sizeof(BOOL)) )
	{
		MessageBoxT(NULL, "setting TCP_NODELAY failed!", MB_OK);
	}
	return _hSocket;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
