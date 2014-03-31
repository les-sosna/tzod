// TankClient.h

#pragma once


//#include "Socket.h"
#include "ControlPacket.h"
#include "ClientBase.h"

/////////////////////////////////////////////////////////

//struct NetworkStats
//{
//	size_t  bytesSent;
//	size_t  bytesRecv;
//	size_t  bytesPending;
//};

struct ControlPacket;
struct PlayerDesc;
struct BotDesc;
struct ILevelController;

class Peer;
class Variant;


class TankClient : public ClientBase
{
public:
	SafePtr<Peer> _peer;
	ControlPacketVector _ctrl;
	float _boost;
	bool _hasCtrl;
	bool _gameStarted;

	void OnDisconnect(Peer *, int err);

public:
	int _latency;

private:
//	NetworkStats _stats;
	ILevelController *_levelController;

public:
	TankClient(ILevelController *levelController);
	~TankClient();

	void Connect(const std::string &hostaddr);

	void SendPlayerReady(bool ready);
	void SendAddBot(const BotDesc &bot);
	void SendPlayerInfo(const PlayerDesc &pd);
	void SendTextMessage(const std::string &msg);

//	void GetStatistics(NetworkStats *pStats);



	Delegate<void()> eventPlayersUpdate;
	Delegate<void()> eventStartGame;
	Delegate<void(size_t, bool)> eventPlayerReady;
	Delegate<void(const std::string&)> eventTextMessage;
	Delegate<void(const std::string&)> eventErrorMessage;
	Delegate<void()> eventConnected;

	// IClient
	virtual bool SupportPause() const { return false; }
	virtual bool SupportEditor() const { return false; }
	virtual bool SupportSave() const { return false; }
	virtual bool IsLocal() const { return false; }
	virtual void SendControl(const ControlPacket &cp);
	virtual bool RecvControl(ControlPacketVector &result);
    virtual const char* GetActiveProfile() const;

private:
	// remote functions
	void ClTextMessage(Peer *from, int task, const Variant &arg);
	void ClErrorMessage(Peer *from, int task, const Variant &arg);
	void ClGameInfo(Peer *from, int task, const Variant &arg);
	void ClPlayerQuit(Peer *from, int task, const Variant &arg);
	void ClControl(Peer *from, int task, const Variant &arg);
	void ClPlayerReady(Peer *from, int task, const Variant &arg);
	void ClStartGame(Peer *from, int task, const Variant &arg);
	void ClAddHuman(Peer *from, int task, const Variant &arg);
	void ClAddBot(Peer *from, int task, const Variant &arg);
	void ClSetPlayerInfo(Peer *from, int task, const Variant &arg);
	void ClSetBoost(Peer *from, int task, const Variant &arg);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
