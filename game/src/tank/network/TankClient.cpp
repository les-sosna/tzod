// TankClient.cpp
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TankClient.h"

#include "core/debug.h"
#include "core/Console.h"
#include "core/Application.h"

#include "config/Config.h"
#include "config/Language.h"



///////////////////////////////////////////////////////////////////////////////

TankClient::TankClient(void)
  : _frame(0)
  , _clientId(0)
{
	ZeroMemory(&_stats, sizeof(NetworkStats));
}

TankClient::~TankClient(void)
{
	ShutDown();
}

void TankClient::Connect(const string_t &hostaddr)
{
	g_app->InitNetwork();


	//
	// get ip addres
	//

	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(g_conf->sv_port->GetInt());

	// try to convert string to ip addres
	addr.sin_addr.s_addr = inet_addr(hostaddr.c_str());

	if( addr.sin_addr.s_addr == INADDR_NONE )
	{
		// try to resolve string as host name
		hostent* he = gethostbyname(hostaddr.c_str());
		if( NULL == he )
		{
			int err = WSAGetLastError();
			TRACE("cl: ERROR - Unable to resolve IP-address (%u)\n", err);
			OnConnect(err ? err : -1);
			return;
		}
		addr.sin_addr.s_addr = *((u_long*)he->h_addr_list[0]);
	}


	//
	// connect
	//

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if( INVALID_SOCKET == s )
	{
		int err = WSAGetLastError();
		TRACE("cl: ERROR - Unable to create socket (%u)\n", WSAGetLastError());
		OnConnect(err ? err : -1);
		return;
	}

	TRACE("cl: connecting to %s\n", inet_ntoa(addr.sin_addr));
	Message(g_lang->net_msg_connecting->Get());

	_peer = WrapRawPtr(new Peer(s));
	_peer->eventConnect.bind(&TankClient::OnConnect, this);
	_peer->eventRecv.bind(&TankClient::OnRecv, this);
	_peer->Connect(&addr);
}

void TankClient::OnConnect(int err)
{
	if( err )
	{
		Message(g_lang->net_msg_connection_failed->Get(), true);
	}
	else
	{
		Message(g_lang->net_msg_connection_established->Get());
	}
}

void TankClient::OnRecv(Peer *who, const DataBlock &db)
{
	_ASSERT(DBTYPE_UNKNOWN != db.type());
	_ASSERT(eventNewData);

	switch( db.type() )
	{
		case DBTYPE_CONTROLPACKET:
			for( int i = 0; i < NET_MULTIPLER-1; i++ )
			{
				INVOKE(eventNewData) (db);   // duplicate packet
			}
			break;
		case DBTYPE_GAMEINFO:
			_latency = db.cast<GameInfo>().latency;
			break;
		case DBTYPE_YOURID:
			_clientId = db.cast<DWORD>();
			return;
	}

	INVOKE(eventNewData) (db);
}

void TankClient::ShutDown()
{
	if( _peer )
	{
		_peer->Close();
		_peer = NULL;
	}
}

void TankClient::Message(const string_t &msg, bool err)
{
	OnRecv(NULL, DataWrap(msg, err ? DBTYPE_ERRORMSG : DBTYPE_TEXTMESSAGE));
}

void TankClient::SendDataToServer(const DataBlock &data)
{
	_ASSERT(DBTYPE_UNKNOWN != data.type());
	_ASSERT(_peer);
	_peer->Send(data);
}

void TankClient::SendControl(const ControlPacket &cp)
{
	if( g_conf->sv_latency->GetInt() < _latency && _latency > 1 )
	{
		--_latency;
		TRACE("cl: packet skipped\n");
		return;     // skip packet
	}

	if( 0 == (_frame % NET_MULTIPLER) )
	{
		SendDataToServer(DataWrap(cp, DBTYPE_CONTROLPACKET));
	}
	_frame++;


	if( g_conf->sv_latency->GetInt() > _latency )
	{
		++_latency;
		SendControl(cp); // duplicate packet
		TRACE("cl: packet duplicated\n");
	}
}

void TankClient::GetStatistics(NetworkStats *pStats)
{
	memcpy(pStats, &_stats, sizeof(NetworkStats));
}

///////////////////////////////////////////////////////////////////////////////
// end of file
