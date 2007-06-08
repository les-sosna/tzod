// TankServer.h

#include "stdafx.h"
#include "TankServer.h"

#include "core/debug.h"
#include "core/console.h"

///////////////////////////////////////////////////////////////////////////////

TankServer::TankServer(void)
{
	_logFile = fopen("server_trace.txt", "w");
	_ASSERT(_logFile);

	_bInit = false;
	_evStopListen  = NULL;
	_hAcceptThread = NULL;

	InitializeCriticalSection(&_csClients);

	InitializeCriticalSection(&_MainCS);
	_hMainSemaphore = NULL;
	_hMainStopEvent = NULL;
	_hMainThread    = NULL;

	_nextFreeId = 0x1000;
}

TankServer::~TankServer(void)
{
	DeleteCriticalSection(&_MainCS);
	DeleteCriticalSection(&_csClients);

	_ASSERT( NULL == _evStopListen );
	if( INVALID_SOCKET != _socketListen ) _socketListen.Close();
	//----------------------------------------
	if( _bInit ) WSACleanup();
	fclose(_logFile);
}

void TankServer::ShutDown()
{
	if( _hMainThread )
	{
		_ASSERT(_hMainStopEvent);
		_ASSERT(_hMainSemaphore);

		SetEvent(_hMainStopEvent);
		WaitForSingleObject(_hMainThread, INFINITE);

		CloseHandle(_hMainStopEvent);
		CloseHandle(_hMainSemaphore);
		CloseHandle(_hMainThread);

		_hMainStopEvent = NULL;
		_hMainSemaphore = NULL;
		_hMainThread    = NULL;

        _ASSERT(NULL == _hAcceptThread);
		_ASSERT(NULL == _hClientsEmptyEvent);
		_ASSERT(NULL == _evStopListen);
	}
}

void TankServer::SendMainThreadData(DWORD id_from, const DataBlock &data)
{
	_ASSERT(DBTYPE_UNKNOWN != data.type());
	EnterCriticalSection(&_MainCS);
	_MainData.push(MainThreadData());
	_MainData.back().data    = data;
	_MainData.back().id_from = id_from;
	LeaveCriticalSection(&_MainCS);
	ReleaseSemaphore(_hMainSemaphore, 1, NULL);
}

void TankServer::SendClientThreadData(const std::list<ClientDesc>::iterator &it, const DataBlock &data)
{
	_ASSERT(DBTYPE_UNKNOWN != data.type());
	EnterCriticalSection(&it->cs);
	it->data.push(data);
	LeaveCriticalSection(&it->cs);
	ReleaseSemaphore(it->semaphore, 1, NULL);
}

bool TankServer::init(const LPGAMEINFO pGameInfo)
{
	_ASSERT(!_bInit);

	memcpy(&_GameInfo, pGameInfo, sizeof(GAMEINFO));
	_ASSERT(VERSION == _GameInfo.dwVersion);


	TRACE("Server startup...\n");
	WSAData wsad;
	if( !_bInit )
	{
		if( WSAStartup(0x0002, &wsad) )
			TRACE("ERROR: Windows sockets init failed\n");
	}
	_bInit = true;

	_socketListen = socket(PF_INET, SOCK_STREAM, 0);
	if( INVALID_SOCKET == _socketListen )
	{
		TRACE("ERROR: Unable to create socket\n");
        return false;
	}


	sockaddr_in addr = {0};
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(GAME_PORT);
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

	_hMainSemaphore = CreateSemaphore(NULL, 0, 0xFFFF, NULL); _ASSERT(NULL != _hMainSemaphore);
	_hMainStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	TRACE("Server is online\n");

	DWORD tmp;
	_hAcceptThread  = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) AcceptProc, this, 0, &tmp);
	_hMainThread    = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) MainProc, this, 0, &tmp);
	SetThreadPriority(_hMainThread, THREAD_PRIORITY_ABOVE_NORMAL);

	return true;
}

DWORD WINAPI TankServer::ClientProc(ClientThreadData *pData)
{
	int result;

	try
	{
		DataBlock db = DataWrap(pData->pServer->_GameInfo, DBTYPE_GAMEINFO);

		// отправка информации о сервере
		result = pData->it->s.Send(pData->it->stop, db.raw_data(), db.raw_size());
		if( result ) throw result;


		//
		// получение данных игрока
		//

		DataBlock new_player = DataBlock(sizeof(PlayerDescEx));
		result = pData->it->s.Recv(pData->it->stop, new_player.raw_data(), new_player.raw_size());
		if( result ) throw result;
		memcpy(&pData->it->desc, new_player.data(), sizeof(PlayerDesc));



		//
		// присвоение клиенту нового ID
		//

		db = DataWrap(pData->it->id, DBTYPE_YOURID);
		result = pData->it->s.Send(pData->it->stop, db.raw_data(), db.raw_size());
		if( result ) throw result;



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
					pde.dwNetworkId = it->id;

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
			((PlayerDescEx *) new_player.data())->dwNetworkId = pData->it->id;
			pData->pServer->SendMainThreadData(pData->it->id, new_player);
		}
        LeaveCriticalSection(&pData->pServer->_csClients);



		//
		// основной цикл
		//
		//                    Aborted + 0         Aborted + 1
		HANDLE handles[2] = {pData->it->stop, pData->it->semaphore};
        while( true )
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
				case DBTYPE_NEWPLAYER:
				{
					_ASSERT(sizeof(PlayerDesc) == db.size());
					if( sizeof(PlayerDesc) != db.size() ) break;
					PlayerDescEx pde;
					memcpy(&pde, db.data(), sizeof(PlayerDesc)); // copy PlayerDesc part only
					EnterCriticalSection(&pData->pServer->_csClients);
					pde.dwNetworkId = ++pData->pServer->_nextFreeId;
					LeaveCriticalSection(&pData->pServer->_csClients);
					db = DataBlock(sizeof(PlayerDescEx));
					db.type() = DBTYPE_NEWPLAYER;
					db.cast<PlayerDescEx>() = pde;
					pData->pServer->SendMainThreadData(pData->it->id, db);
				} break;
				case DBTYPE_CONTROLPACKET:
					pData->pServer->SERVER_TRACE("control packet from %d\n", pData->it->id);
					EnterCriticalSection(&pData->pServer->_csClients);
					pData->it->ctrl.push( db.cast<ControlPacket>() );
					pData->pServer->TrySendFrame();
					LeaveCriticalSection(&pData->pServer->_csClients);
					break;
				case DBTYPE_PLAYERREADY:
				case DBTYPE_PLAYERQUIT:
				case DBTYPE_TEXTMESSAGE:
					pData->pServer->SendMainThreadData(pData->it->id, db);
					break;
				case DBTYPE_PING:
					pData->it->s.Send(pData->it->stop, db.raw_data(), db.raw_size());

				//	pData->pServer->SendClientThreadData(pData->it, db);
				//	pData->pServer->SendMainThreadData(pData->it->id, db);
					break;
				default:
					_ASSERT(FALSE);
					throw (int) Socket::Error;
				}
			} break;
			/////////////////////////////////////////////////////////////////
			case Socket::Aborted + 1: // поступили данные для отправки в сеть
			{
				EnterCriticalSection(&pData->it->cs);
				_ASSERT(!pData->it->data.empty());
				DataBlock db(pData->it->data.front());
				pData->it->data.pop();
				LeaveCriticalSection(&pData->it->cs);
				//-----------------
				int res = pData->it->s.Send(pData->it->stop, db.raw_data(), db.raw_size());
				if( res ) throw res;
			} break;
			/////////////////////////////////////////////////////////////////
			default:
				_ASSERT(FALSE);
			}
		}
	}
	catch( int )
	{
	}


	//
	// извещение о выходе игрока
	//

	if( pData->it->connected )
	{
		EnterCriticalSection(&pData->pServer->_csClients);
		pData->it->connected = false;
		LeaveCriticalSection(&pData->pServer->_csClients);

		DataBlock db(sizeof(DWORD));
		db.type() = DBTYPE_PLAYERQUIT;
		db.cast<DWORD>() = pData->it->id;
		pData->pServer->SendMainThreadData(pData->it->id, db);
	}


	//
	// освобождение ресурсов
	//

	EnterCriticalSection(&pData->pServer->_csClients);
	{
	//	pData->it->s.SetEvents(FD_CLOSE);
	//	shutdown(pData->it->s, SD_SEND);
	//	pData->it->s.Wait(pData->it->stop);
		if( INVALID_SOCKET != pData->it->s )
			pData->it->s.Close();
		WSACloseEvent(pData->it->stop);
		DeleteCriticalSection(&pData->it->cs);
		CloseHandle(pData->it->semaphore);
	//	CloseHandle(pData->it->thread);
		pData->pServer->_clients.erase(pData->it);
		if( pData->pServer->_clients.empty() )
		{
			_ASSERT(NULL != pData->pServer->_hClientsEmptyEvent);
			SetEvent(pData->pServer->_hClientsEmptyEvent);
		}
	}
	LeaveCriticalSection(&pData->pServer->_csClients);

	delete pData;
	return 0;
}

DWORD WINAPI TankServer::AcceptProc(TankServer *pServer)
{
	while( true )
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
				pctd->it->semaphore = CreateSemaphore(NULL, 0, 0xFFFF, NULL);
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
		}
	}

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
				SERVER_TRACE("нет пакета от %d\n", it->id);
				break; // от кого-то не поступили данные
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

DWORD WINAPI TankServer::MainProc(TankServer *pServer)
{
	HANDLE handles[2] = { pServer->_hMainSemaphore, pServer->_hMainStopEvent };
	while( WAIT_OBJECT_0 == WaitForMultipleObjects(2, handles, FALSE, INFINITE) )
	{
		EnterCriticalSection(&pServer->_MainCS);
		_ASSERT(!pServer->_MainData.empty());
		DataBlock db      = pServer->_MainData.front().data;
		DWORD     id_from = pServer->_MainData.front().id_from;
		pServer->_MainData.pop();
		LeaveCriticalSection(&pServer->_MainCS);
        //---------------------------------------
		std::list<ClientDesc>::iterator it;
		EnterCriticalSection(&pServer->_csClients);
		switch( db.type() )
		{
		case DBTYPE_TEXTMESSAGE:
		{
			string_t msg = "<";
			it = pServer->_clients.begin();
			for( ; it != pServer->_clients.end(); ++it )
			{
				if( it->id == id_from )
				{
					msg += it->desc.nick;
					break;
				}
			}
			_ASSERT(it != pServer->_clients.end());
			msg += "> ";
			msg += (char *) db.data();

			DataBlock db_new(msg.size()+1);
			db_new.type() = DBTYPE_TEXTMESSAGE;
			strcpy((char *) db_new.data(), msg.c_str());

			it = pServer->_clients.begin();
			for( ; it != pServer->_clients.end(); ++it )
				pServer->SendClientThreadData(it, db_new);
		} break;
		case DBTYPE_PLAYERREADY:
		{
			BOOL bAllPlayersReady = TRUE;
			it = pServer->_clients.begin();
			for( ; it != pServer->_clients.end(); ++it )
			{
				if( it->id == id_from )	it->ready = db.cast<dbPlayerReady>().ready;
				if( !it->ready ) bAllPlayersReady = FALSE;
				pServer->SendClientThreadData(it, db);
			}
			if( bAllPlayersReady )
			{
				// запрещение приема новых подключений
				SetEvent(pServer->_evStopListen);
				WaitForSingleObject(pServer->_hAcceptThread, INFINITE);
				CloseHandle(pServer->_evStopListen);
				CloseHandle(pServer->_hAcceptThread);
				pServer->_hAcceptThread = NULL;
				pServer->_evStopListen = NULL;


				string_t msg = "Все игроки готовы. Запуск игры...";
				DataBlock tmp(msg.size()+1);
				tmp.type() = DBTYPE_TEXTMESSAGE;
				strcpy((char *) tmp.data(), msg.c_str());
				it = pServer->_clients.begin();
				for( ; it != pServer->_clients.end(); ++it )
					pServer->SendClientThreadData(it, tmp);
				//------------------------------------------
				tmp = DataBlock();
				tmp.type() = DBTYPE_STARTGAME;
				it = pServer->_clients.begin();
				for( ; it != pServer->_clients.end(); ++it )
					pServer->SendClientThreadData(it, tmp);
			}
		} break;
		case DBTYPE_NEWPLAYER:
			it = pServer->_clients.begin();
			for( ; it != pServer->_clients.end(); ++it )
				pServer->SendClientThreadData(it, db);
			break;
		case DBTYPE_PLAYERQUIT:
			it = pServer->_clients.begin();
			for( ; it != pServer->_clients.end(); ++it )
				if( id_from != it->id )
					pServer->SendClientThreadData(it, db);
			pServer->TrySendFrame();
			break;
		case DBTYPE_PING:
			it = pServer->_clients.begin();
			for( ; it != pServer->_clients.end(); ++it )
			//	if( id_from == it->id )
					pServer->SendClientThreadData(it, db);
			break;
		case DBTYPE_CHECKSUM:

		default:
			_ASSERT(FALSE);
		}
		LeaveCriticalSection(&pServer->_csClients);
	}


	//
	// ожидание завершения прослушивающего потока
	//

	if( pServer->_hAcceptThread )
	{
		_ASSERT(NULL != pServer->_evStopListen);
		SetEvent(pServer->_evStopListen);
		WaitForSingleObject(pServer->_hAcceptThread, INFINITE);
		CloseHandle(pServer->_evStopListen);
		CloseHandle(pServer->_hAcceptThread);
		pServer->_hAcceptThread = NULL;
		pServer->_evStopListen = NULL;
	}


	//
	// ожидание завершения клиентских потоков
	//

	EnterCriticalSection(&pServer->_csClients);
	{
		std::list<ClientDesc>::iterator it = pServer->_clients.begin();
		for( ; it != pServer->_clients.end(); ++it )
		{
			_ASSERT( WSA_INVALID_EVENT != it->stop );
			SetEvent( it->stop );
		}
	}
	LeaveCriticalSection(&pServer->_csClients);

	WaitForSingleObject(pServer->_hClientsEmptyEvent, INFINITE);
	CloseHandle(pServer->_hClientsEmptyEvent);
	pServer->_hClientsEmptyEvent = NULL;

	return 0;
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
