// TankServer.h

#pragma once

#include "Socket.h"
#include "datablock.h"
#include "TankClient.h"


class TankServer
{
	bool _init;
	GameInfo _gameInfo;

	struct ClientDesc
	{
		DWORD      id;               // уникальный идентификатор клиента
		Socket     s;                // дескриптор сокета
		WSAEVENT   stop;             // событие означает отмену сетевой транзакции
		//---------------------------//----------------------------
		PlayerDesc desc;             // описание игрока
		BOOL       connected;        // флаг определяет, что поле desc корректно
		BOOL       ready;            // игрок готов начать игру
		//---------------------------//----------------------------
		HANDLE evData;               // событие означает наличие данных в data
		CRITICAL_SECTION cs;         // критическая секция защищает поля data и ctrl
		std::vector<DataBlock> data; // данные для отправки в сеть
		std::queue<ControlPacket> ctrl;// поступившие из сети данные управления
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


	bool TrySendFrame();         // отправка кадра если все данные получены
	static DWORD WINAPI MainProc(TankServer *pServer);

	void SendClientThreadData(const std::list<ClientDesc>::iterator &it, const DataBlock &data);

public:
	TankServer(void);
	~TankServer(void);

	bool init(const GameInfo *info);
	void ShutDown();
};

///////////////////////////////////////////////////////////////////////////////
// end of file
