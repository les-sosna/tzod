// Peer.cpp

#include "stdafx.h"

#include "Peer.h"

#include "core/debug.h"
#include "core/Application.h"

///////////////////////////////////////////////////////////////////////////////

Peer::Peer(SOCKET s)
  : _in(false)
  , _out(true)
  , _socket(s)
  , _paused(false)
  , _readyToSend(true)
{
	if( _socket.SetEvents(FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE) )
	{
		TRACE("peer: ERROR - Unable to select event (%u)", WSAGetLastError());
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
		TRACE("peer: ERROR - connect call failed!");
		return WSAGetLastError();
	}
	if( WSAEWOULDBLOCK != WSAGetLastError() )
	{
		TRACE("peer: error %d; WSAEWOULDBLOCK expected!", WSAGetLastError());
		return WSAGetLastError();
	}
	return 0;
}

void Peer::Post(int func, const Variant &arg)
{
	_out.EntityBegin();
	_out & func;
	_out & const_cast<Variant &>(arg);
	_out.EntityEnd();

	if( _readyToSend )
	{
		TrySend();
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
		TRACE("peer: EnumNetworkEvents error 0x%08x", WSAGetLastError());
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
			TRACE("peer: read error 0x%08x", ne.iErrorCode[FD_READ_BIT]);
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, ne.iErrorCode[FD_READ_BIT]);
			return;
		}

		int result = _in.Recv(_socket);
		if( result < 0 )
		{
			TRACE("peer: unexpected error 0x%08x", WSAGetLastError());
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, WSAGetLastError());
			return;
		}
		else if( 0 == result )
		{
			// connection was gracefully closed
			TRACE("peer: connection closed by remote side");
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, 0);
			return;
		}
		else
		{
			while( _in.EntityProbe() )
			{
				_pendingCalls.push(PendingRemoteCall());
				PendingRemoteCall &pc = _pendingCalls.back();

				_in.EntityBegin();
				int func;
				_in & func;
				HandlersMap::const_iterator it = _handlers.find(func);
				if( _handlers.end() == it )
				{
					_pendingCalls.pop();
					TRACE("peer: invalid function code");
					assert(eventDisconnect);
					INVOKE(eventDisconnect) (this, 0);
					return;
				}
				pc.handler = it->second.handler;
				pc.arg.ChangeType(it->second.argType);
				_in & pc.arg;
				_in.EntityEnd();
			}

			if( !_paused )
			{
				ProcessInput();
			}
		}
	}

	if( ne.lNetworkEvents & FD_WRITE )
	{
		if( ne.iErrorCode[FD_WRITE_BIT] )
		{
			TRACE("peer: write error 0x%08x", ne.iErrorCode[FD_WRITE_BIT]);
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, ne.iErrorCode[FD_WRITE_BIT]);
			return;
		}
		_readyToSend = true;
		if( !_out.IsEmpty() )
		{
			TrySend();
		}
	}
}

void Peer::ProcessInput()
{
	while( !_pendingCalls.empty() )
	{
		if( _paused )
		{
			break;
		}

		PendingRemoteCall pc(_pendingCalls.front());
		_pendingCalls.pop();

		INVOKE(pc.handler) (this, -1, pc.arg);
	}
}

bool Peer::TrySend()
{
	assert(_readyToSend);
	size_t sent;
	if( int err = _out.Send(_socket, &sent) )
	{
		if( WSAEWOULDBLOCK == err )
		{
			_readyToSend = false;
		}
		else
		{
			TRACE("peer: network error %u", err);
			assert(eventDisconnect);
			INVOKE(eventDisconnect) (this, err);
			return false;
		}
	}
	_sentRecent += sent;
	return true;
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
