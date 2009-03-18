// TankServer.h

#include "stdafx.h"

#include "core/debug.h"
#include "core/console.h"
#include "core/Application.h"

#include "config/Config.h"
#include "config/Language.h"

#include "TankServer.h"
#include "ServerFunctions.h"
#include "datablock.h"

#include "LobbyClient.h"

///////////////////////////////////////////////////////////////////////////////

PeerServer::PeerServer(SOCKET s_)
  : Peer(s_)
{
}

///////////////////////////////////////////////////////////////////////////////

TankServer::TankServer(const SafePtr<LobbyClient> &announcer)
  : _nextFreeId(0x1000)
  , _connectedCount(0)
  , _frameReadyCount(0)
  , _announcer(announcer)
{
	TRACE("sv: Server created\n");
}

TankServer::~TankServer(void)
{
	TRACE("sv: Server is shutting down\n");


	//
	// cancel lobby registration
	//

	if( _announcer )
	{
		_announcer->Cancel();
	}


	//
	// close listener
	//

	if( INVALID_SOCKET != _socketListen )
	{
		_socketListen.Close();
	}


	//
	// disconnect clients
	//

	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		(*it)->Close();
	}


	TRACE("sv: Server destroyed\n");
}

bool TankServer::init(const GameInfo *info)
{
	g_app->InitNetwork();

	_gameInfo = *info;
	_ASSERT(VERSION == _gameInfo.dwVersion);


	TRACE("sv: Server init...\n");
	_socketListen = socket(PF_INET, SOCK_STREAM, 0);
	if( INVALID_SOCKET == _socketListen )
	{
		TRACE("sv: ERROR - Unable to create socket\n");
		return false;
	}


	sockaddr_in addr = {0};
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(g_conf->sv_port->GetInt());
	addr.sin_family      = AF_INET;

	if( bind(_socketListen, (sockaddr *) &addr, sizeof(sockaddr_in)) )
	{
		TRACE("sv: ERROR - Unable to bind socket: %d\n", WSAGetLastError());
		return false;
	}

	if( listen(_socketListen, SOMAXCONN) )
	{
		TRACE("sv: ERROR - Listen call failed\n");
		return false;
	}

	if( _socketListen.SetEvents(FD_ACCEPT) )
	{
		TRACE("sv: ERROR - Unable to select event (%u)\n", WSAGetLastError());
		return false;
	}

	_socketListen.SetCallback(CreateDelegate(&TankServer::OnListenerEvent, this));

	TRACE("sv: Server is online\n");

	if( _announcer )
	{
		_announcer->AnnounceHost(g_conf->sv_port->GetInt());
	}

	return true;
}

void TankServer::OnListenerEvent()
{
	WSANETWORKEVENTS ne = {0};
	if( _socketListen.EnumNetworkEvents(&ne) )
	{
		TRACE("sv: EnumNetworkEvents error 0x%08x\n", WSAGetLastError());
		return;
	}
	_ASSERT(ne.lNetworkEvents & FD_ACCEPT);

	if( 0 != ne.iErrorCode[FD_ACCEPT_BIT] )
	{
		TRACE("sv: accept error 0x%08x\n", ne.iErrorCode[FD_ACCEPT_BIT]);
		return;
	}


	SOCKET s = accept(_socketListen, NULL, NULL);
	if( INVALID_SOCKET == s )
	{
		TRACE("sv: accept call returned error 0x%08x\n", WSAGetLastError());
		return;
	}


	TRACE("sv: Client connected\n");


	//
	// Add new client
	//

	_clients.push_back(SafePtr<PeerServer>(new PeerServer(s)));
	PeerServer &cl = *GetRawPtr(_clients.back());

//	cl.RegisterHandler(SV_POST_TEXTMESSAGE, CreateDelegate(&TankServer::TextMessage, this));

	cl.connected = FALSE;
	cl.ready = FALSE;
	cl.id = ++_nextFreeId;
	cl.eventRecv.bind(&TankServer::OnRecv, this);
	cl.eventDisconnect.bind(&TankServer::OnDisconnect, this);

	// send server info
	cl.Send(DataWrap(_gameInfo, DBTYPE_GAMEINFO));

	// send new client ID
	cl.Send(DataWrap(cl.id, DBTYPE_YOURID));
}

void TankServer::OnRecv(Peer *who_, const DataBlock &db)
{
	_ASSERT(dynamic_cast<PeerServer*>(who_));
	PeerServer *who = static_cast<PeerServer*>(who_);

	switch( db.type() )
	{
		case DBTYPE_CONTROLPACKET:
		{
			who->ctrl.push(db.cast<ControlPacket>());
			if( 1 == who->ctrl.size() )
			{
				++_frameReadyCount;
				if( _frameReadyCount == _connectedCount )
				{
					who->ctrl.front().wControlState |= MISC_YOUARETHELAST;
					SendFrame();
				}
			}
			break;
		}

		case DBTYPE_TEXTMESSAGE:
		{
			string_t msg = "<";
			for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				if( (*it)->id == who->id )
				{
					msg += (*it)->desc.nick;
					break;
				}
			}
			_ASSERT(msg.length() > 1);
			msg += "> ";
			msg += (char *) db.Data();

			DataBlock db_new = DataWrap(msg, DBTYPE_TEXTMESSAGE);
			for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
				(*it)->Send(db_new);

			break;
		}

		case DBTYPE_PLAYERREADY:
		{
			bool bAllPlayersReady = true;
			for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				if( (*it)->id == who->id )
				{
					(*it)->ready = db.cast<dbPlayerReady>().ready;
				}
				if( !(*it)->ready )
				{
					bAllPlayersReady = false;
				}
				(*it)->Send(db);
			}

			if( bAllPlayersReady )
			{
				// запрещение приема новых подключений
				_socketListen.Close();
				if( _announcer )
				{
					_announcer->Cancel();
				}

				DataBlock tmp = DataWrap(g_lang->net_msg_starting_game->Get(), DBTYPE_TEXTMESSAGE);
				for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
				{
					(*it)->Send(tmp);
				}

				tmp = DataBlock();
				tmp.type() = DBTYPE_STARTGAME;
				for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
				{
					(*it)->Send(tmp);
				}
			}
			break;
		}
/*
		case DBTYPE_PLAYERQUIT:
		{
			for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				if( who->id != (*it)->id )
				{
					(*it)->Send(db);
				}
			}
			SendFrame();
			break;
		}
*/
		case DBTYPE_PING:
		{
			who->Send(db); // send packet back
			break;
		}

		case DBTYPE_NEWBOT:
		{
			for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				(*it)->Send(db);
			}
			break;
		}

		case DBTYPE_PLAYERINFO:
		{
			who->desc = db.cast<PlayerDesc>();


			//
			// tell newly connected player about other players
			//

			for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				if( (*it)->connected )
				{
					PlayerDescEx pde;
					memcpy(&pde, &(*it)->desc, sizeof(PlayerDesc)); // copy PlayerDesc part of PlayerDescEx
					pde.id = (*it)->id;
					who->Send(DataWrap(pde, DBTYPE_NEWPLAYER));
				}
			}
			who->connected = TRUE;
			++_connectedCount;


			//
			// tell other players about newly connected player
			//

			PlayerDescEx pde;
			memcpy(&pde, &who->desc, sizeof(PlayerDesc)); // copy PlayerDesc part of PlayerDescEx
			pde.id = who->id;
			for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				(*it)->Send(DataWrap(pde, DBTYPE_NEWPLAYER));
			}

			break;
		}

		default:
			_ASSERT(FALSE);
			// TODO: disconnect client
	}
}

void TankServer::OnDisconnect(Peer *who_, int err)
{
	TRACE("sv: client disconnected\n");

	_ASSERT(dynamic_cast<PeerServer*>(who_));
	PeerServer *who = static_cast<PeerServer*>(who_);

	if( who->connected )
	{
		who->connected = false;
		--_connectedCount;
		if( !who->ctrl.empty() )
		{
			--_frameReadyCount;
		}

		DataBlock db(sizeof(DWORD));
		db.type() = DBTYPE_PLAYERQUIT;
		db.cast<DWORD>() = who->id;

		for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
		{
			if( who->id != (*it)->id )
			{
				(*it)->Send(db);
			}
		}

		if( _frameReadyCount == _connectedCount )
		{
			SendFrame();
		}
	}

	who->Close();
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		if( (*it) == who )
		{
			_clients.erase(it);
			break;
		}
	}
}

void TankServer::SendFrame()
{
	_ASSERT(_frameReadyCount == _connectedCount);

	DataBlock db(sizeof(ControlPacket) * _connectedCount);
	db.type() = DBTYPE_CONTROLPACKET;

	for( PeerList::iterator it1 = _clients.begin(); it1 != _clients.end(); it1++ )
	{
		if( (*it1)->connected )
		{
			int i = 0;
			for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				if( !(*it)->connected ) continue;
				const ControlPacket &cp = (*it)->ctrl.front();
				db.cast<ControlPacket>(_connectedCount - (++i)) = cp;

#ifdef NETWORK_DEBUG
				FrameToCSMap::_Pairib ib = _frame2cs.insert(FrameToCSMap::value_type(cp.frame, cp.checksum));
				if( !ib.second && ib.first->second != cp.checksum )
				{
					TRACE("sv: sync error detected at frame %u!\n", cp.frame);
					char buf[256];
					wsprintf(buf, "sync error at frame %u: 0x%x 0x%x", cp.frame, ib.first->second, cp.checksum);
					MessageBox(g_env.hMainWnd, buf, TXT_VERSION, MB_ICONERROR);
					ExitProcess(-1);
				}
#endif
			}
			_ASSERT(_connectedCount * sizeof(ControlPacket) == db.DataSize());
			(*it1)->Send(db);
		}
	}

	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		if( (*it)->connected )
		{
			(*it)->ctrl.pop();
			if( (*it)->ctrl.empty() )
			{
				_ASSERT(_frameReadyCount > 0);
				--_frameReadyCount;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file
