// TankServer.h

#pragma once

#include "Peer.h"

class PeerServer : public Peer
{
public:
	DWORD      id;               // уникальный идентификатор клиента
	PlayerDesc desc;             // описание игрока
	BOOL       connected;        // флаг определяет, что поле desc корректно
	BOOL       ready;            // игрок готов начать игру
	std::queue<ControlPacket> ctrl;// поступившие из сети данные управления

	PeerServer(SOCKET s_);
};

///////////////////////////////////////////////////////////////////////////////

class TankServer
{
	GameInfo _gameInfo;

	DWORD _nextFreeId;

	typedef std::list<SafePtr<PeerServer> >  PeerList;
	PeerList _clients;


	Socket _socketListen;


	bool TrySendFrame();         // отправка кадра если все данные получены
	static DWORD WINAPI MainProc(TankServer *pServer);

	void OnListenerEvent();
	void OnRecv(Peer *who, const DataBlock &db);

public:
	TankServer(void);
	~TankServer(void);

	bool init(const GameInfo *info);
	void ShutDown();
};

///////////////////////////////////////////////////////////////////////////////
// end of file
