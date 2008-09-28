// TankClient.cpp
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TankClient.h"

#include "core/debug.h"
#include "core/Console.h"

#include "config/Config.h"
#include "config/Language.h"

#include "gc/Vehicle.h" // FIXME!



///////////////////////////////////////////////////////////////////////////////

ControlPacket::ControlPacket()
{
	ZeroMemory(this, sizeof(*this));
}

void ControlPacket::fromvs(const VehicleState &vs)
{
	wControlState = 0;
	weap = 0;
	body = 0;

	wControlState |= STATE_MOVEFORWARD * (false != vs._bState_MoveForward);
	wControlState |= STATE_MOVEBACK    * (false != vs._bState_MoveBack);
	wControlState |= STATE_FIRE        * (false != vs._bState_Fire);
	wControlState |= STATE_ALLOWDROP   * (false != vs._bState_AllowDrop);
	wControlState |= STATE_ENABLELIGHT * (false != vs._bLight);

	if( vs._bExplicitBody )
	{
		body = (unsigned short) (int(vs._fBodyAngle / PI2 * 65536.0f + 0.5f) & 0xffff);
		wControlState |= MODE_EXPLICITBODY;
	}
	else
	{
		wControlState |= STATE_ROTATELEFT  * (false != vs._bState_RotateLeft);
		wControlState |= STATE_ROTATERIGHT * (false != vs._bState_RotateRight);
	}

	if( vs._bExplicitTower )
	{
		weap = (unsigned short) (int(vs._fTowerAngle / PI2 * 65536.0f + 0.5f) & 0xffff);
		wControlState |= MODE_EXPLICITTOWER;
	}
	else
	{
		wControlState |= STATE_TOWERLEFT   * (false != vs._bState_TowerLeft);
		wControlState |= STATE_TOWERRIGHT  * (false != vs._bState_TowerRight);
		wControlState |= STATE_TOWERCENTER * (false != vs._bState_TowerCenter);
	}
}

void ControlPacket::tovs(VehicleState &vs) const
{
	ZeroMemory(&vs, sizeof(VehicleState));

	vs._bState_MoveForward = (0 != (wControlState & STATE_MOVEFORWARD));
	vs._bState_MoveBack    = (0 != (wControlState & STATE_MOVEBACK));
	vs._bState_Fire        = (0 != (wControlState & STATE_FIRE));
	vs._bState_AllowDrop   = (0 != (wControlState & STATE_ALLOWDROP));

	vs._bLight         = (0 != (wControlState & STATE_ENABLELIGHT));

	vs._bExplicitBody  = (0 != (wControlState & MODE_EXPLICITBODY));
	vs._bExplicitTower = (0 != (wControlState & MODE_EXPLICITTOWER));

	if( vs._bExplicitBody)
	{
		vs._fBodyAngle = (float) body / 65536.0f * PI2;
	}
	else
	{
		vs._bState_RotateLeft  = (0 != (wControlState & STATE_ROTATELEFT));
		vs._bState_RotateRight = (0 != (wControlState & STATE_ROTATERIGHT));
	}

	if( vs._bExplicitTower)
		vs._fTowerAngle = (float) weap / 65536.0f * PI2;
	else
	{
		vs._bState_TowerLeft   = (0 != (wControlState & STATE_TOWERLEFT));
		vs._bState_TowerRight  = (0 != (wControlState & STATE_TOWERRIGHT));
		vs._bState_TowerCenter = (0 != (wControlState & STATE_TOWERCENTER));
	}
}

///////////////////////////////////////////////////////////////////////////////

TankClient::TankClient(void)
{
	_init = false;
	_hwnd = NULL;

	//---------------------------------
	ZeroMemory(&_stats, sizeof(NetworkStats));
	ZeroMemory(&_lastPacket, sizeof(ControlPacket));

	_frame              = 0;
	_clientId           = 0;
	_buf_incoming_size  = 0;
	_buf_outgoing_size  = 0;

	_readyToSend = false;
	_gameStarted = false;
}

TankClient::~TankClient(void)
{
	ShutDown();
	if( _init ) WSACleanup();
}

bool TankClient::Connect(const string_t &hostaddr, HWND hMainWnd)
{
	TRACE("Connecting...\n");

	if( !_init )
	{
		TRACE("Init windows sockets\n");
		WSAData wsad;
		if( WSAStartup(0x0002, &wsad) )
		{
			TRACE("ERROR: Windows sockets init failed\n");
			return false;
		}
		_init = true;
		TRACE("Windows sockets initialized\n");
	}

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if( INVALID_SOCKET == _socket )
	{
		TRACE("ERROR: Unable to create socket\n");
		return false;
	}
	TRACE("Socket created\n");

	try
	{
		sockaddr_in addr;
		ZeroMemory(&addr, sizeof(sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_port   = htons(g_conf->sv_port->GetInt());

		// пробуем преобразовать в числовой IP-адрес из строкового
		addr.sin_addr.s_addr = inet_addr(hostaddr.c_str());

		if( addr.sin_addr.s_addr == INADDR_NONE )
		{
			// Host не IP-адрес, может это имя хоста?
			hostent* he = gethostbyname(hostaddr.c_str());
			if( NULL == he )
			{
				TRACE("ERROR: Unable to resolve IP-address\n");
				throw false;
			}
			addr.sin_addr.s_addr = *((u_long*)he->h_addr_list[0]);
		}

		_hwnd = hMainWnd;
		if( WSAAsyncSelect(_socket, _hwnd, WM_CUSTOMCLIENTMSG, FD_CONNECT|FD_READ|FD_WRITE) )
		{
			TRACE("ERROR: Unable to select event\n");
			throw false;
		}

		if( !connect(_socket, (sockaddr *) &addr, sizeof(sockaddr_in)) )
		{
			TRACE("ERROR: connect call failed!\n");
			throw false;
		}

		int error;
		if( WSAEWOULDBLOCK != (error = WSAGetLastError()) )
		{
			TRACE("error %d; WSAEWOULDBLOCK expected!\n", error);
			throw false;
		}
	}
	catch(bool error)
	{
		_socket.Close();
		return error;
	}

	Message(g_lang->net_msg_connecting->Get());

	return true;
}

bool TankClient::recv_all()
{
	int count = recv(_socket, _buf_incoming + _buf_incoming_size, MAX_BUFFER_SIZE - _buf_incoming_size, 0);
	if( SOCKET_ERROR == count && WSAEWOULDBLOCK != WSAGetLastError() )
	{
		return false;
	}
	DataBlock tmp;
	_buf_incoming_size += count;
    void *bufptr = _buf_incoming;
	while( tmp.from_stream(&bufptr, &_buf_incoming_size) )
	{
		NewData(tmp);
	}
	memmove(_buf_incoming, bufptr, _buf_incoming_size);
	return true;
}

bool TankClient::send_all()
{
	if( _outgoing.empty() || !_readyToSend ) return true;
	while( _buf_outgoing_size + _outgoing.front().raw_size() <= MAX_BUFFER_SIZE )
	{
		memcpy((char*) _buf_outgoing + _buf_outgoing_size,
			_outgoing.front().raw_data(), _outgoing.front().raw_size());
		_buf_outgoing_size += _outgoing.front().raw_size();
		_outgoing.pop();
		if( _outgoing.empty() ) break;
	}
	int count = send(_socket, _buf_outgoing, _buf_outgoing_size, 0);
	if( SOCKET_ERROR == count )
	{
		_readyToSend = false;
		return (WSAEWOULDBLOCK == WSAGetLastError());
	}
	_stats.bytesSent += count;
	_buf_outgoing_size -= count;
	memmove(_buf_outgoing, _buf_outgoing + count, _buf_outgoing_size);
	if( !_outgoing.empty() && 0 == _buf_outgoing_size && 0 != count )
		return send_all();
	return true;
}

LRESULT TankClient::Mirror(WPARAM wParam, LPARAM lParam)
{
	if( WSAGETSELECTERROR(lParam) )
	{
		TRACE("network error: %d\n", WSAGETSELECTERROR(lParam));
		Message(g_lang->net_msg_connection_failed->Get(), true);
		return 0;
	}

	///////////////////////////////////////////

	switch( WSAGETSELECTEVENT(lParam) )
	{
	case FD_READ:
		recv_all();
		break;
	case FD_WRITE:
		_readyToSend = true;
		if( !_outgoing.empty() )
			send_all();
		break;
	case FD_CONNECT:
		Message(g_lang->net_msg_connection_established->Get());
		break;
	default:
		_ASSERT(FALSE);
	}

	return 0;
}

void TankClient::ShutDown()
{
	if( INVALID_SOCKET != _socket )
	{
		WSAAsyncSelect( _socket, _hwnd, 0, 0 ); // turn off notifications
		u_long ulParam = 0;
		ioctlsocket( _socket, FIONBIO, &ulParam );  // return back to blocking mode
		_socket.Close();
	}
}

void TankClient::Message(const string_t &msg, bool err)
{
	DataBlock db(msg.size() + 1);
	memcpy(db.data(), msg.c_str(), db.size());
	db.type() = err ? DBTYPE_ERRORMSG : DBTYPE_TEXTMESSAGE;
	NewData(db);
}

bool TankClient::GetData(DataBlock &data)
{
	if( !_incoming.empty() )
	{
		data = _incoming.front();
		_incoming.pop();
		if( DBTYPE_CONTROLPACKET == data.type() )
		{
			--_stats.nFramesInBuffer;
		}
		return true;
	}
	return false;
}

void TankClient::NewData(const DataBlock &data)
{
	_ASSERT(DBTYPE_UNKNOWN != data.type());
	//-------------------------------
	// внутренний фильтр
	switch( data.type() )
	{
	case DBTYPE_CONTROLPACKET:
		_stats.nFramesInBuffer++;
		for( int i = 0; i < NET_MULTIPLER-1; i++ ) // duplicate packet
		{
			_incoming.push(data);
			_stats.nFramesInBuffer++;
		}
		break;
	case DBTYPE_GAMEINFO:
		_latency = data.cast<GameInfo>().latency;
		break;
	case DBTYPE_STARTGAME:
		break;
	case DBTYPE_YOURID:
		_clientId = data.cast<DWORD>();
		return;
	}
	//-------------------------------
	_incoming.push(data);
	_stats.bytesRecv += data.raw_size();
}

void TankClient::SendDataToServer(const DataBlock &data)
{
	_ASSERT(DBTYPE_UNKNOWN != data.type());
	_outgoing.push(data);
	send_all();
}

void TankClient::SendControl(const ControlPacket &cp)
{
	if( g_conf->sv_latency->GetInt() < _latency && _latency > 1 )
	{
		--_latency;
		TRACE("packet skipped\n");
		return;     // skip packet
	}

	if( 0 == (_frame % NET_MULTIPLER) )
	{
		//if( cp == _lastPacket )
		//{
		//	DataBlock db(0);
		//	db.type() = DBTYPE_CONTROLPACKET;
		//	SendDataToServer(db);
		//}
		//else
		//{
			_lastPacket = cp;
			DataBlock db(sizeof(ControlPacket));
			db.type() = DBTYPE_CONTROLPACKET;
			memcpy(db.data(), &cp, db.size());
			SendDataToServer(db);
		//}
	}
	_frame++;


	if( g_conf->sv_latency->GetInt() > _latency )
	{
		++_latency;
		SendControl(cp); // duplicate packet
		TRACE("packet duplicated\n");
	}
}

void TankClient::GetStatistics(NetworkStats *pStats)
{
	memcpy(pStats, &_stats, sizeof(NetworkStats));
}

bool TankClient::RecvControl(ControlPacket &cp)
{
	if( _ctrlBuf.empty() )
		return false;
	cp = _ctrlBuf.front();
	_ctrlBuf.pop();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
