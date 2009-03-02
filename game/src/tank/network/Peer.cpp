// Peer.cpp

#include "stdafx.h"

#include "Peer.h"
#include "datablock.h"

#include "core/debug.h"
#include "core/Console.h"
#include "core/Application.h"

///////////////////////////////////////////////////////////////////////////////

Peer::Peer(SOCKET s)
{
	_socket = s;

	if( _socket.SetEvents(FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE) )
	{
		TRACE("peer: ERROR - Unable to select event (%u)\n", WSAGetLastError());
		throw std::runtime_error("peer: Unable to select event");
	}

	Delegate<void()> d;
	d.bind(&Peer::OnSocketEvent, this);
	_socket.SetCallback(d);
}

Peer::~Peer()
{
	_ASSERT(INVALID_SOCKET == _socket);
}

void Peer::Close()
{
	_ASSERT(INVALID_SOCKET != _socket);
	_socket.Close();
}

void Peer::Connect(const sockaddr_in *addr)
{
	_ASSERT(INVALID_SOCKET != _socket);
	_ASSERT(eventConnect);
	if( !connect(_socket, (sockaddr *) addr, sizeof(sockaddr_in)) )
	{
		TRACE("cl: ERROR - connect call failed!\n");
		INVOKE(eventConnect) (WSAGetLastError());
		return;
	}
	int error;
	if( WSAEWOULDBLOCK != (error = WSAGetLastError()) )
	{
		TRACE("cl: error %d; WSAEWOULDBLOCK expected!\n", error);
		INVOKE(eventConnect) (error);
	}
}

void Peer::Send(const DataBlock &db)
{
	_ASSERT(INVALID_SOCKET != _socket);

	if( _outgoing.empty() )
	{
		// try sending immediately if outgoing queue is empty
		size_t sent = 0;
		do
		{
			int result = send(_socket, db.RawData() + sent, db.RawSize() - sent, 0);
			if( SOCKET_ERROR == result )
			{
				// schedule delayed sending rest of data
				int err = WSAGetLastError();
				if( WSAEWOULDBLOCK != err )
				{
					TRACE("peer: network error %u\n", err);
					_ASSERT(eventDisconnect);
					INVOKE(eventDisconnect) (this, err);
					return;
				}
				_outgoing.insert(_outgoing.end(), db.RawData() + sent, db.RawData() + db.RawSize());
				break;
			}
			else
			{
				_ASSERT(result > 0);
				sent += result;
			}
		} while( sent < db.RawSize() );
	}
	else
	{
		// schedule sending whole data if outgoing queue is not empty
		_outgoing.insert(_outgoing.end(), db.RawData(), db.RawData() + db.RawSize());
	}
}

void Peer::OnSocketEvent()
{
	_ASSERT(INVALID_SOCKET != _socket);

	WSANETWORKEVENTS ne = {0};
	if( _socket.EnumNetworkEvents(&ne) )
	{
		TRACE("peer: EnumNetworkEvents error 0x%08x\n", WSAGetLastError());
		_ASSERT(eventDisconnect);
		INVOKE(eventDisconnect) (this, WSAGetLastError());
		return;
	}

	if( ne.lNetworkEvents & FD_CONNECT )
	{
		_ASSERT(eventConnect);
		INVOKE(eventConnect) (ne.iErrorCode[FD_CONNECT_BIT]);
	}

	if( ne.lNetworkEvents & FD_CLOSE )
	{
		_ASSERT(eventDisconnect);
		INVOKE(eventDisconnect) (this, ne.iErrorCode[FD_CLOSE_BIT]);
	}

	if( ne.lNetworkEvents & FD_READ )
	{
		if( ne.iErrorCode[FD_READ_BIT] )
		{
			TRACE("peer: read error 0x%08x\n", ne.iErrorCode[FD_READ_BIT]);
			_ASSERT(eventDisconnect);
			INVOKE(eventDisconnect) (this, ne.iErrorCode[FD_READ_BIT]);
			return;
		}

		char buf[1024];
		int result = recv(_socket, buf, sizeof(buf), 0);

		if( result < 0 )
		{
			TRACE("peer: unexpected error 0x%08x\n", WSAGetLastError());
			_ASSERT(eventDisconnect);
			INVOKE(eventDisconnect) (this, WSAGetLastError());
			return;
		}
		else if( 0 == result )
		{
			// connection was gracefully closed
			TRACE("peer: connection closed by remote side\n");
			_ASSERT(eventDisconnect);
			INVOKE(eventDisconnect) (this, 0);
			return;
		}
		else
		{
			DataBlock db;
			if( _incoming.empty() )
			{
				size_t parsedTotal = 0;
				while( size_t parsed = db.Parse(buf + parsedTotal, result - parsedTotal) )
				{
					parsedTotal += parsed;
					_ASSERT(eventRecv);
					INVOKE(eventRecv) (this, db);
				}
				if( parsedTotal < (unsigned) result )
				{
					_incoming.insert(_incoming.end(), buf + parsedTotal, buf + result);
				}
			}
			else
			{
				_incoming.insert(_incoming.end(), buf, buf + result);
				size_t parsedTotal = 0;
				while( size_t parsed = db.Parse(&_incoming.front() + parsedTotal, _incoming.size() - parsedTotal) )
				{
					parsedTotal += parsed;
					_ASSERT(eventRecv);
					INVOKE(eventRecv) (this, db);
				}
				_incoming.erase(_incoming.begin(), _incoming.begin() + parsedTotal);
			}
		}
	}

	if( ne.lNetworkEvents & FD_WRITE )
	{
		if( ne.iErrorCode[FD_WRITE_BIT] )
		{
			TRACE("peer: write error 0x%08x\n", ne.iErrorCode[FD_WRITE_BIT]);
			_ASSERT(eventDisconnect);
			INVOKE(eventDisconnect) (this, ne.iErrorCode[FD_WRITE_BIT]);
			return;
		}

	//	_ASSERT(!outgoing.empty());
		if( _outgoing.empty() )
		{
			TRACE("nothing to send\n");
			return;
		}

		size_t sent = 0;
		do
		{
			int result = send(_socket, &_outgoing.front() + sent, _outgoing.size() - sent, 0);
			if( SOCKET_ERROR == result )
			{
				int err = WSAGetLastError();
				if( WSAEWOULDBLOCK != err )
				{
					TRACE("peer: network error %u\n", err);
					_ASSERT(eventDisconnect);
					INVOKE(eventDisconnect) (this, err);
					return;
				}
				break;
			}
			else
			{
				_ASSERT(result > 0);
				sent += result;
			}
		} while( sent < _outgoing.size() );

		// remove sent bytes
		_ASSERT(sent <= _outgoing.size());
		_outgoing.erase(_outgoing.begin(), _outgoing.begin() + sent);
	}
}


// end of file
