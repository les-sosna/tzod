// Peer.cpp

#include "stdafx.h"

#include "Peer.h"

#include "core/debug.h"
#include "core/Console.h"
#include "core/Application.h"

///////////////////////////////////////////////////////////////////////////////

Peer::Peer(SOCKET s)
  : _in(false)
  , _out(true)
  , _socket(s)
  , _paused(false)
{
	if( _socket.SetEvents(FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE) )
	{
		TRACE("peer: ERROR - Unable to select event (%u)\n", WSAGetLastError());
		throw std::runtime_error("peer: Unable to select event");
	}
	_socket.SetCallback(CreateDelegate(&Peer::OnSocketEvent, this));
}

Peer::~Peer()
{
	assert(INVALID_SOCKET == _socket);
}

void Peer::Close()
{
	assert(INVALID_SOCKET != _socket);
	_socket.Close();
}

int Peer::Connect(const sockaddr_in *addr)
{
	assert(INVALID_SOCKET != _socket);
	if( !connect(_socket, (sockaddr *) addr, sizeof(sockaddr_in)) )
	{
		TRACE("cl: ERROR - connect call failed!\n");
		return WSAGetLastError();
	}
	if( WSAEWOULDBLOCK != WSAGetLastError() )
	{
		TRACE("cl: error %d; WSAEWOULDBLOCK expected!\n", WSAGetLastError());
		return WSAGetLastError();
	}
	return 0;
}

void Peer::Post(int func, const Variant &arg)
{
	bool trySend = _out.IsEmpty();

	_out.EntityBegin();
	_out & func;
	_out & const_cast<Variant &>(arg);
	_out.EntityEnd();

	if( trySend )
	{
		if( int err = _out.Send(_socket) )
		{
			TRACE("peer: network error %u\n", err);
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, err);
		}
	}
}

void Peer::RegisterHandler(int func, Variant::TypeId argType, HandlerProc handler)
{
	assert(0 == _handlers.count(func));
	_handlers[func].argType = argType;
	_handlers[func].handler = handler;
}

void Peer::OnSocketEvent()
{
	assert(INVALID_SOCKET != _socket);

	WSANETWORKEVENTS ne = {0};
	if( _socket.EnumNetworkEvents(&ne) )
	{
		TRACE("peer: EnumNetworkEvents error 0x%08x\n", WSAGetLastError());
		assert(eventDisconnect);
		INVOKE(eventDisconnect) (this, WSAGetLastError());
		return;
	}

	if( ne.lNetworkEvents & FD_CONNECT )
	{
		if( ne.iErrorCode[FD_CONNECT_BIT] )
		{
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, ne.iErrorCode[FD_CONNECT_BIT]);
		}
	}

	if( ne.lNetworkEvents & FD_CLOSE )
	{
		assert(eventDisconnect);
		INVOKE(eventDisconnect) (this, ne.iErrorCode[FD_CLOSE_BIT]);
	}

	if( ne.lNetworkEvents & FD_READ )
	{
		if( ne.iErrorCode[FD_READ_BIT] )
		{
			TRACE("peer: read error 0x%08x\n", ne.iErrorCode[FD_READ_BIT]);
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, ne.iErrorCode[FD_READ_BIT]);
			return;
		}

		int result = _in.Recv(_socket);
		if( result < 0 )
		{
			TRACE("peer: unexpected error 0x%08x\n", WSAGetLastError());
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, WSAGetLastError());
			return;
		}
		else if( 0 == result )
		{
			// connection was gracefully closed
			TRACE("peer: connection closed by remote side\n");
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, 0);
			return;
		}
		else
		{
			ProcessInput();
		}
	}

	if( ne.lNetworkEvents & FD_WRITE )
	{
		if( ne.iErrorCode[FD_WRITE_BIT] )
		{
			TRACE("peer: write error 0x%08x\n", ne.iErrorCode[FD_WRITE_BIT]);
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, ne.iErrorCode[FD_WRITE_BIT]);
			return;
		}

		if( int err = _out.Send(_socket) )
		{
			TRACE("peer: network error %u\n", err);
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, err);
			return;
		}
	}
}

void Peer::ProcessInput()
{
	while( _in.EntityProbe() )
	{
		if( _paused )
		{
			break;
		}

		int func;
		Variant arg;
		_in.EntityBegin();
		_in & func;
		HandlersMap::const_iterator it = _handlers.find(func);
		assert(_handlers.end() != it);
		arg.ChangeType(it->second.argType);
		_in & arg;
		_in.EntityEnd();
		INVOKE(it->second.handler) (this, -1, arg);
	}
}

void Peer::Pause()
{
	assert(!_paused);
	_paused = true;
}

void Peer::Resume()
{
	if( _paused )
	{
		_paused = false;
		ProcessInput();
	}
}

// end of file
