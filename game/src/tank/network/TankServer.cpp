// TankServer.h

#include "stdafx.h"
#include "TankServer.h"

#include "core/debug.h"
#include "core/console.h"

#include "config/Config.h"
#include "config/Language.h"

///////////////////////////////////////////////////////////////////////////////

TankServer::TankServer(void)
{
	_logFile = fopen("server_trace.txt", "w");
	_ASSERT(_logFile);

	_init = false;
	_evStopListen  = NULL;
	_hAcceptThread = NULL;

	InitializeCriticalSection(&_csClients);

	_nextFreeId = 0x1000;
}

TankServer::~TankServer(void)
{
	ShutDown();

	DeleteCriticalSection(&_csClients);

	_ASSERT( NULL == _evStopListen );
	if( INVALID_SOCKET != _socketListen ) _socketListen.Close();
	//----------------------------------------
	if( _init ) WSACleanup();
	fclose(_logFile);

	_ASSERT(NULL == _hAcceptThread);
	_ASSERT(NULL == _hClientsEmptyEvent);
	_ASSERT(NULL == _evStopListen);
}

void TankServer::ShutDown()
{
	//
	// ожидание завершения прослушивающего потока
	//

	if( _hAcceptThread )
	{
		_ASSERT(NULL != _evStopListen);
		SetEvent(_evStopListen);
		WaitForSingleObject(_hAcceptThread, INFINITE);
		CloseHandle(_evStopListen);
		CloseHandle(_hAcceptThread);
		_hAcceptThread = NULL;
		_evStopListen = NULL;
	}


	//
	// ожидание завершения клиентских потоков
	//

	EnterCriticalSection(&_csClients);
	for( std::list<ClientDesc>::iterator it = _clients.begin(); it != _clients.end(); ++it )
	{
		_ASSERT( WSA_INVALID_EVENT != it->stop );
		SetEvent( it->stop );
	}
	LeaveCriticalSection(&_csClients);

	WaitForSingleObject(_hClientsEmptyEvent, INFINITE);
	CloseHandle(_hClientsEmptyEvent);
	_hClientsEmptyEvent = NULL;
}

void TankServer::SendClientThreadData(const std::list<ClientDesc>::iterator &it, const DataBlock &db)
{
	_ASSERT(DBTYPE_UNKNOWN != db.type());
	EnterCriticalSection(&it->cs);
	it->data.push_back(db);
	if( 1 == it->data.size() )
	{
		SetEvent(it->evData);
	}
	LeaveCriticalSection(&it->cs);
}

bool TankServer::init(const GameInfo *info)
{
	_ASSERT(!_init);

	_gameInfo = *info;
	_ASSERT(VERSION == _gameInfo.dwVersion);


	TRACE("Server startup...\n");
	WSAData wsad;
	if( !_init )
	{
		if( WSAStartup(0x0002, &wsad) )
			TRACE("ERROR: Windows sockets init failed\n");
	}
	_init = true;

	_socketListen = socket(PF_INET, SOCK_STREAM, 0);
	if( INVALID_SOCKET == _socketListen )
	{
		TRACE("ERROR: Unable to create socket\n");
		return false;
	}


	sockaddr_in addr = {0};
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(g_conf->sv_port->GetInt());
	addr.sin_family      = AF_INET;

	if( bind(_socketListen, (sockaddr *) &addr, sizeof(sockaddr_in)) )
	{
		TRACE("ERROR: Unable to bind socket: %d\n", WSAGetLastError());
		return false;
	}

	if( listen(_socketListen, SOMAXCONN) )
	{
		TRACE("ERROR: Listen call failed\n");
		return false;
	}

	_evStopListen = CreateEvent(NULL, TRUE, FALSE, NULL);

	if( _socketListen.SetEvents(FD_ACCEPT) )
	{
		TRACE("ERROR: Unable to select event\n");
		return false;
	}

	_hClientsEmptyEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

	TRACE("Server is online\n");

	DWORD tmp;
	_hAcceptThread  = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) AcceptProc, this, 0, &tmp);

	return true;
}

DWORD WINAPI TankServer::ClientProc(ClientThreadData *pData)
{
	int result;

	try
	{
		DataBlock db = DataWrap(pData->pServer->_gameInfo, DBTYPE_GAMEINFO);

		// send server info
		result = pData->it->s.Send(pData->it->stop, db.raw_data(), db.raw_size());
		if( result ) throw result;

		// send new client ID
		db = DataWrap(pData->it->id, DBTYPE_YOURID);
		result = pData->it->s.Send(pData->it->stop, db.raw_data(), db.raw_size());
		if( result ) throw result;

		// recv player info
		DataBlock new_player = DataBlock(sizeof(PlayerDesc));
		result = pData->it->s.Recv(pData->it->stop, new_player.raw_data(), new_player.raw_size());
		if( result ) throw result;
		_ASSERT(new_player.type() == DBTYPE_PLAYERINFO);
		pData->it->desc = new_player.cast<PlayerDesc>();


		//
		// отправка информации об игроках
		//

		EnterCriticalSection(&pData->pServer->_csClients);
		{
			std::list<ClientDesc>::iterator it;


			//
			// отправка данных
			//

			it = pData->pServer->_clients.begin();
			for( ; it != pData->pServer->_clients.end(); ++it )
			{
				if( it->connected )
				{
					PlayerDescEx pde;
					memcpy(&pde, &it->desc, sizeof(PlayerDesc)); // copy PlayerDesc part of PlayerDescEx
					pde.id = it->id;

					db = DataWrap(pde, DBTYPE_NEWPLAYER);
					result = pData->it->s.Send(pData->it->stop, db.raw_data(), db.raw_size());
					if( result )
					{
						LeaveCriticalSection(&pData->pServer->_csClients);
						throw result;
					}
				}
			}
			pData->it->connected = TRUE;


			//
			// извещение остальных игроков о пополнении
			//
			PlayerDescEx pde;
			memcpy(&pde, &pData->it->desc, sizeof(PlayerDesc)); // copy PlayerDesc part of PlayerDescEx
			pde.id = pData->it->id;
			for( it = pData->pServer->_clients.begin(); it != pData->pServer->_clients.end(); ++it )
			{
				pData->pServer->SendClientThreadData(it, DataWrap(pde, DBTYPE_NEWPLAYER));
			}
		}
		LeaveCriticalSection(&pData->pServer->_csClients);



		//
		// основной цикл
		//
		//                    Aborted + 0         Aborted + 1
		HANDLE handles[2] = {pData->it->stop, pData->it->evData};
		for(;;)
		{
			DataBlock::size_type size;
			switch( pData->it->s.Recv(handles, 2, &size, sizeof(DataBlock::size_type)) )
			{
			case Socket::Error:
				throw (int) Socket::Error;
			case Socket::Aborted + 0:
				throw (int) Socket::Aborted;
			/////////////////////////////////////////////////////////////////
			case Socket::OK:          // поступили новые данные из сети
			{
				int res;
				DataBlock db(size);
				res = pData->it->s.Recv(pData->it->stop, &db.type(), sizeof(DataBlock::type_type));
				if( res ) throw res;
				res = pData->it->s.Recv(pData->it->stop, db.data(), size);
				if( res ) throw res;
				//----------------------------
				switch( db.type() )
				{
				case DBTYPE_CONTROLPACKET:
				{
					pData->pServer->SERVER_TRACE("control packet from %d\n", pData->it->id);
					EnterCriticalSection(&pData->pServer->_csClients);
					pData->it->ctrl.push( db.cast<ControlPacket>() );
					pData->pServer->TrySendFrame();
					LeaveCriticalSection(&pData->pServer->_csClients);
					break;
				}

				case DBTYPE_TEXTMESSAGE:
				{
					std::list<ClientDesc>::iterator it;
					string_t msg = "<";
					for( it = pData->pServer->_clients.begin(); it != pData->pServer->_clients.end(); ++it )
					{
						if( it->id == pData->it->id )
						{
							msg += it->desc.nick;
							break;
						}
					}
					_ASSERT(it != pData->pServer->_clients.end());
					msg += "> ";
					msg += (char *) db.data();

					DataBlock db_new(msg.size()+1);
					db_new.type() = DBTYPE_TEXTMESSAGE;
					strcpy((char *) db_new.data(), msg.c_str());

					for( it = pData->pServer->_clients.begin(); it != pData->pServer->_clients.end(); ++it )
						pData->pServer->SendClientThreadData(it, db_new);

					break;
				}

				case DBTYPE_PLAYERREADY:
				{
					std::list<ClientDesc>::iterator it;
					bool bAllPlayersReady = true;
					for( it = pData->pServer->_clients.begin(); it != pData->pServer->_clients.end(); ++it )
					{
						if( it->id == pData->it->id )
						{
							it->ready = db.cast<dbPlayerReady>().ready;
						}
						if( !it->ready )
						{
							bAllPlayersReady = false;
						}
						pData->pServer->SendClientThreadData(it, db);
					}

					if( bAllPlayersReady )
					{
						// запрещение приема новых подключений
						SetEvent(pData->pServer->_evStopListen);
						WaitForSingleObject(pData->pServer->_hAcceptThread, INFINITE);
						CloseHandle(pData->pServer->_evStopListen);
						CloseHandle(pData->pServer->_hAcceptThread);
						pData->pServer->_hAcceptThread = NULL;
						pData->pServer->_evStopListen = NULL;

						string_t msg = g_lang->net_msg_starting_game->Get();
						DataBlock tmp(msg.size()+1);
						tmp.type() = DBTYPE_TEXTMESSAGE;
						strcpy((char *) tmp.data(), msg.c_str());
						it = pData->pServer->_clients.begin();
						for( ; it != pData->pServer->_clients.end(); ++it )
						{
							pData->pServer->SendClientThreadData(it, tmp);
						}

						tmp = DataBlock();
						tmp.type() = DBTYPE_STARTGAME;
						for( it = pData->pServer->_clients.begin(); it != pData->pServer->_clients.end(); ++it )
						{
							pData->pServer->SendClientThreadData(it, tmp);
						}
					}
					break;
				}

				case DBTYPE_PLAYERQUIT:
				{
					std::list<ClientDesc>::iterator it;
					for( it = pData->pServer->_clients.begin(); it != pData->pServer->_clients.end(); ++it )
					{
						if( pData->it->id != it->id )
						{
							pData->pServer->SendClientThreadData(it, db);
						}
					}
					pData->pServer->TrySendFrame();
					break;
				}

				case DBTYPE_PING:
				{
					pData->it->s.Send(pData->it->stop, db.raw_data(), db.raw_size());
					break;
				}

				case DBTYPE_NEWBOT:
				{
					std::list<ClientDesc>::iterator it;
					for( it = pData->pServer->_clients.begin(); it != pData->pServer->_clients.end(); ++it )
					{
						pData->pServer->SendClientThreadData(it, db);
					}
					break;
				}

				default:
					_ASSERT(FALSE);
					throw (int) Socket::Error;
				}
				break;
			}
			/////////////////////////////////////////////////////////////////
			case Socket::Aborted + 1: // поступили данные для отправки в сеть
			{
				EnterCriticalSection(&pData->it->cs);
				_ASSERT(!pData->it->data.empty());
				for( std::vector<DataBlock>::iterator it = pData->it->data.begin();
				     it != pData->it->data.end(); ++it )
				{
					pData->it->s.Accumulate(it->raw_data(), it->raw_size());
				}
				pData->it->data.clear();
				ResetEvent(pData->it->evData);
				LeaveCriticalSection(&pData->it->cs);
				if( int res = pData->it->s.Send_accum(pData->it->stop) )
				{
					throw res;
				}
				break;
			}
			/////////////////////////////////////////////////////////////////
			default:
				_ASSERT(FALSE);
			}
		}
	}
	catch( int )
	{
	}


	EnterCriticalSection(&pData->pServer->_csClients);

	if( pData->it->connected )
	{
		pData->it->connected = false;

		DataBlock db(sizeof(DWORD));
		db.type() = DBTYPE_PLAYERQUIT;
		db.cast<DWORD>() = pData->it->id;

		std::list<ClientDesc>::iterator it;
		for( it = pData->pServer->_clients.begin(); it != pData->pServer->_clients.end(); ++it )
		{
			if( pData->it->id != it->id )
			{
				pData->pServer->SendClientThreadData(it, db);
			}
		}
		pData->pServer->TrySendFrame();
	}


//	pData->it->s.SetEvents(FD_CLOSE);
//	shutdown(pData->it->s, SD_SEND);
//	pData->it->s.Wait(pData->it->stop);
	if( INVALID_SOCKET != pData->it->s )
		pData->it->s.Close();
	WSACloseEvent(pData->it->stop);
	DeleteCriticalSection(&pData->it->cs);
	CloseHandle(pData->it->evData);
	pData->pServer->_clients.erase(pData->it);
	if( pData->pServer->_clients.empty() )
	{
		_ASSERT(NULL != pData->pServer->_hClientsEmptyEvent);
		SetEvent(pData->pServer->_hClientsEmptyEvent);
	}

	LeaveCriticalSection(&pData->pServer->_csClients);

	delete pData;
	return 0;
}

DWORD WINAPI TankServer::AcceptProc(TankServer *pServer)
{
	for(;;)
	{
		int result = pServer->_socketListen.Wait(pServer->_evStopListen);
		if( result ) break;

		if( !pServer->_socketListen.CheckEvent(FD_ACCEPT_BIT) )
			break;

		SOCKET s = accept(pServer->_socketListen, NULL, NULL);
		if( INVALID_SOCKET != s )
		{
			//
			// подготовка и запуск клиентского потока
			//

			HANDLE hThread = NULL;
			ClientThreadData *pctd = new ClientThreadData;
			pctd->pServer = pServer;
			EnterCriticalSection(&pServer->_csClients);
			{
				DWORD tmp;
				pServer->_clients.push_back(ClientDesc());
				pctd->it = pServer->_clients.end(); pctd->it--;
				pctd->it->connected = FALSE;
				pctd->it->ready     = FALSE;
				pctd->it->s         = s;
				pctd->it->id        = ++pServer->_nextFreeId;
				pctd->it->stop      = WSACreateEvent();
				pctd->it->evData    = CreateEvent(NULL, TRUE, FALSE, NULL);
				hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ClientProc,
				/*------------------*/ pctd, CREATE_SUSPENDED, &tmp);
				SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
				_ASSERT(NULL != pServer->_hClientsEmptyEvent);
				ResetEvent(pServer->_hClientsEmptyEvent);
				InitializeCriticalSection(&pctd->it->cs);
			}
			LeaveCriticalSection(&pServer->_csClients);

			_ASSERT(WSA_INVALID_EVENT != pctd->it->stop);
			_ASSERT(NULL != hThread);

			ResumeThread(hThread);
			CloseHandle(hThread);
		}
	} // for(;;)

	pServer->_socketListen.Close();

	return 0;
}

bool TankServer::TrySendFrame()
{
	SERVER_TRACE("call TrySendFrame()\n");

	//
	// определяем сколько у нас получено кадров
	//
	int count = 0;
	std::list<ClientDesc>::iterator it = _clients.begin();
	for( ; it != _clients.end(); ++it )
	{
		if( it->connected )
		{
			if( it->ctrl.empty() )
			{
				SERVER_TRACE("no packet from %d\n", it->id);
				break;
			}
			count++;
		}
	}

	if( it != _clients.end() )
		return false;


	//
	// все данные получены. рассылаем кадры клиентам
	//

	DataBlock db_new(sizeof(ControlPacket) * count);
	db_new.type() = DBTYPE_CONTROLPACKET;

	std::list<ClientDesc>::iterator it1 = _clients.begin();
	for( ; it1 != _clients.end(); it1++ )
	{
		if( it1->connected )
		{
			it = _clients.begin();

			#ifdef NETWORK_DEBUG
			DWORD chsum;
			bool first = true;
			#endif


			for( int i = 0; it != _clients.end(); ++it )
			{
				if( !it->connected ) continue;
				db_new.cast<ControlPacket>(count - (++i)) = it->ctrl.front();

				#ifdef NETWORK_DEBUG
				if( first )
				{
					chsum = it->ctrl.front().checksum;
					first = false;
				}
				else
				{
					DWORD tmp = it->ctrl.front().checksum;
					if( tmp != chsum )
					{
						SERVER_TRACE("sync error detected!\n");
						char buf[128];
						wsprintf(buf, "sync error: 0x%x 0x%x", tmp, chsum);
						MessageBox(g_env.hMainWnd, buf, TXT_VERSION, MB_ICONERROR);
						ExitProcess(-1);
					}
				}
				#endif
			}
			_ASSERT(count * sizeof(ControlPacket) == db_new.size());
			SERVER_TRACE("send frame to %d...\n", it1->id);
			SendClientThreadData(it1, db_new);
			SERVER_TRACE("OK\n");
		}
	}

	//
	// очистка буферов клиентов
	//
	for( it = _clients.begin(); it != _clients.end(); ++it )
		if( it->connected )
			it->ctrl.pop();


	SERVER_TRACE("sent frames to all clients OK\n");

	return true;
}


void TankServer::SERVER_TRACE(const char *fmt, ...)
{
#ifndef NETWORK_DEBUG
	return;
#endif

	fprintf(_logFile, "0x%X: ", GetCurrentThreadId());

	va_list va;
	va_start(va, fmt);
	vfprintf(_logFile, fmt, va);
	va_end(va);
	fflush(_logFile);
}


///////////////////////////////////////////////////////////////////////////////
// end of file
