// TankServer.h

#pragma once

#include "Socket.h"
#include "datablock.h"
#include "TankClient.h"


class TankServer
{
	FILE *_logFile;
	void SERVER_TRACE(const char *fmt, ...);

	bool _init;
	GAMEINFO _GameInfo;

	struct ClientDesc
	{
		DWORD      id;               // уникальный идентификатор клиента
		Socket     s;                // дескриптор сокета
		WSAEVENT   stop;             // событие означает отмену сетевой транзакции
		//---------------------------//----------------------------
		BOOL       connected;        // флаг определяет, что поле desc корректно
		PlayerDesc desc;             // описание игрока
		BOOL       ready;            // игрок готов начать игру
		//---------------------------//----------------------------
		HANDLE semaphore;            // семафор контролирует наличие данных в очереди
		CRITICAL_SECTION cs;         // критическая секция защищает поля data и ctrl
		std::queue<DataBlock> data;  // данные, которые должны быть обработаны потоком
		std::queue<ControlPacket> ctrl;// поступившие от игрока данные управления
	};


	DWORD _nextFreeId;

	std::list<ClientDesc> _clients;
	CRITICAL_SECTION    _csClients; // защищает _clients
	HANDLE     _hClientsEmptyEvent; // событие в сигнальном состоянии, когда список _clients пуст

	struct ClientThreadData
	{
		TankServer *pServer;
		std::list<ClientDesc>::iterator it;
	};

	static DWORD WINAPI ClientProc(ClientThreadData *pData);


	Socket _socketListen;        // прослушивающий сокет
	HANDLE _evStopListen;        // событие отмены прослушивания

	HANDLE _hAcceptThread;       // поток, принимающий подключения клиентов
	static DWORD WINAPI AcceptProc(TankServer *pServer);


	HANDLE _hMainThread;         // основной серверный поток
	HANDLE _hMainSemaphore;      // семафор контролирует наличие данных в очереди _MainData
	HANDLE _hMainStopEvent;      // событие завершает основной поток
	CRITICAL_SECTION _MainCS;    // критическая секция защищает данные _MainData

	struct MainThreadData
	{
		DWORD id_from;
		DataBlock data;
	};
	std::queue<MainThreadData> _MainData;   // данные, которые должны быть обработаны основным потоком

	bool TrySendFrame();         // отправка кадра если все данные получены
	static DWORD WINAPI MainProc(TankServer *pServer);

	void SendMainThreadData(DWORD id_from, const DataBlock &data);
	void SendClientThreadData(const std::list<ClientDesc>::iterator &it, const DataBlock &data);

public:
	TankServer(void);
	~TankServer(void);

	bool init(const LPGAMEINFO pGameInfo);
	void ShutDown();
};

///////////////////////////////////////////////////////////////////////////////
// end of file
