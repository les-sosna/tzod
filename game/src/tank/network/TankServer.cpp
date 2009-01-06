// TankServer.h

#include "stdafx.h"

#include "TankServer.h"
#include "datablock.h"

#include "core/debug.h"
#include "core/console.h"
#include "core/Application.h"

#include "config/Config.h"
#include "config/Language.h"

///////////////////////////////////////////////////////////////////////////////

PeerServer::PeerServer(SOCKET s_)
  : Peer(s_)
{
}

///////////////////////////////////////////////////////////////////////////////

TankServer::TankServer(void)
//  : _evStopListen(NULL)
//  , _hAcceptThread(NULL)
  : _nextFreeId(0x1000)
{
	TRACE("sv: Server created\n");
}

TankServer::~TankServer(void)
{
	ShutDown();
	TRACE("sv: Server destroyed\n");
}

void TankServer::ShutDown()
{
	TRACE("sv: Server is shutting down\n");

	//
	// close listener
	//

	if( INVALID_SOCKET != _socketListen )
	{
		g_app->UnregisterHandle(_socketListen.GetEvent());
		_socketListen.Close();
	}


	//
	// disconnect clients
	//

	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		(*it)->Close();
	}
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

	Delegate<void()> d;
	d.bind(&TankServer::OnListenerEvent, this);
	g_app->RegisterHandle(_socketListen.GetEvent(), d);

	TRACE("sv: Server is online\n");

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
	cl.connected = FALSE;
	cl.ready = FALSE;
	cl.id = ++_nextFreeId;
	cl.eventRecv.bind(&TankServer::OnRecv, this);

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
			TrySendFrame();
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
				g_app->UnregisterHandle(_socketListen.GetEvent());
				_socketListen.Close();

//						SetEvent(pData->pServer->_evStopListen);
//						WaitForSingleObject(pData->pServer->_hAcceptThread, INFINITE);
//						CloseHandle(pData->pServer->_evStopListen);
//						CloseHandle(pData->pServer->_hAcceptThread);
//						pData->pServer->_hAcceptThread = NULL;
//						pData->pServer->_evStopListen = NULL;

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

		case DBTYPE_PLAYERQUIT:
		{
			for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
			{
				if( who->id != (*it)->id )
				{
					(*it)->Send(db);
				}
			}
			TrySendFrame();
			break;
		}

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

/*
DWORD WINAPI TankServer::ClientProc(ClientThreadData *pData)
{
//
// disconnect
//
	if( pData->it->connected )
	{
		pData->it->connected = false;

		DataBlock db(sizeof(DWORD));
		db.type() = DBTYPE_PLAYERQUIT;
		db.cast<DWORD>() = pData->it->id;

		std::list<Peer>::iterator it;
		for( it = pData->pServer->_clients.begin(); it != pData->pServer->_clients.end(); ++it )
		{
			if( pData->it->id != it->id )
			{
				pData->pServer->SendClientThreadData(it, db);
			}
		}
		pData->pServer->TrySendFrame();
	}

	if( INVALID_SOCKET != pData->it->s )
		pData->it->s.Close();

	pData->pServer->_clients.erase(pData->it);

	return 0;
}
*/


bool TankServer::TrySendFrame()
{
	//
	// определяем сколько у нас получено кадров
	//
	size_t count = 0;
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		if( (*it)->connected )
		{
			if( (*it)->ctrl.empty() )
			{
				return false;
			}
			++count;
		}
	}


	//
	// все данные получены. рассылаем кадры клиентам
	//

	DataBlock db_new(sizeof(ControlPacket) * count);
	db_new.type() = DBTYPE_CONTROLPACKET;

	for( PeerList::iterator it1 = _clients.begin(); it1 != _clients.end(); it1++ )
	{
		if( (*it1)->connected )
		{
			PeerList::iterator it = _clients.begin();

			#ifdef NETWORK_DEBUG
			DWORD chsum;
			bool first = true;
			#endif


			for( int i = 0; it != _clients.end(); ++it )
			{
				if( !(*it)->connected ) continue;
				db_new.cast<ControlPacket>(count - (++i)) = (*it)->ctrl.front();

				#ifdef NETWORK_DEBUG
				if( first )
				{
					chsum = (*it)->ctrl.front().checksum;
					first = false;
				}
				else
				{
					DWORD tmp = (*it)->ctrl.front().checksum;
					if( tmp != chsum )
					{
						TRACE("sv: sync error detected!\n");
						char buf[128];
						wsprintf(buf, "sync error: 0x%x 0x%x", tmp, chsum);
						MessageBox(g_env.hMainWnd, buf, TXT_VERSION, MB_ICONERROR);
						ExitProcess(-1);
					}
				}
				#endif
			}
			_ASSERT(count * sizeof(ControlPacket) == db_new.DataSize());
			(*it1)->Send(db_new);
		}
	}

	//
	// очистка буферов клиентов
	//
	for( PeerList::iterator it = _clients.begin(); it != _clients.end(); ++it )
		if( (*it)->connected )
			(*it)->ctrl.pop();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
