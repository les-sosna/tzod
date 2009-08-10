// TankClient.h

#pragma once


#include "Socket.h"
#include "ControlPacket.h"


/////////////////////////////////////////////////////////

struct NetworkStats
{
	size_t  bytesSent;
	size_t  bytesRecv;
	size_t  bytesPending;
};

struct ControlPacket;
struct PlayerDesc;
struct BotDesc;

class Peer;
class Variant;


class TankClient
{
	SafePtr<Peer> _peer;
	int _frame;
	ControlPacketVector _ctrl;
	float _boost;
	bool _hasCtrl;
	bool _gameStarted;

	void OnDisconnect(Peer *, int err);

public:
	int _latency;

private:
	unsigned short _clientId;
	NetworkStats _stats;

public:
	TankClient(void);
	~TankClient(void);

	unsigned short GetId() const { return _clientId; }
	float GetBoost() const { return _boost; }

	void Connect(const string_t &hostaddr);
	void ShutDown();

	void SendPlayerReady(bool ready);
	void SendAddBot(const BotDesc &bot);
	void SendPlayerInfo(const PlayerDesc &pd);
	void SendTextMessage(const std::string &msg);
	void SendControl(const ControlPacket &cp); // вызов функции завершает кадр
	bool RecvControl(ControlPacketVector &result);

	void GetStatistics(NetworkStats *pStats);



	Delegate<void()> eventPlayersUpdate;
	Delegate<void()> eventStartGame;
	Delegate<void(unsigned short, bool)> eventPlayerReady;
	Delegate<void(const std::string&)> eventTextMessage;
	Delegate<void(const std::string&)> eventErrorMessage;
	Delegate<void()> eventConnected;


private:
	// remote functions
	bool ClTextMessage(Peer *from, int task, const Variant &arg);
	bool ClErrorMessage(Peer *from, int task, const Variant &arg);
	bool ClGameInfo(Peer *from, int task, const Variant &arg);
	bool ClSetId(Peer *from, int task, const Variant &arg);
	bool ClPlayerQuit(Peer *from, int task, const Variant &arg);
	bool ClControl(Peer *from, int task, const Variant &arg);
	bool ClPlayerReady(Peer *from, int task, const Variant &arg);
	bool ClStartGame(Peer *from, int task, const Variant &arg);
	bool ClAddBot(Peer *from, int task, const Variant &arg);
	bool ClSetPlayerInfo(Peer *from, int task, const Variant &arg);
	bool ClSetBoost(Peer *from, int task, const Variant &arg);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
