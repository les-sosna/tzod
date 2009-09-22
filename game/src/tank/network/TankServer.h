// TankServer.h

#pragma once

#include "Peer.h"
#include "ControlPacket.h"
#include "CommonTypes.h"

#include "core/BitCounter.h"


class PeerServer : public Peer
{
public:
	unsigned short      id;
	ControlPacket       ctrl;
	PlayerDesc          desc;
	int                 svlatency;
	float               clboost;
	BitCounter<128>     leading;
	bool                descValid;
	bool                ctrlValid;
	bool                ready;

	PeerServer(SOCKET s_);
};

///////////////////////////////////////////////////////////////////////////////

class LobbyClient;

class TankServer
{
	GameInfo _gameInfo;

	unsigned short _nextFreeId;

	typedef std::list<SafePtr<PeerServer> >  PeerList;
	PeerList _clients;

	typedef std::pair<int, Variant> PostType;
	std::vector<PostType> _players; // including bots

	int _connectedCount;
	int _frameReadyCount;     // how much clients have ctrl data in buffer


	Socket _socketListen;

	SafePtr<LobbyClient> _announcer;

#ifdef NETWORK_DEBUG
	typedef std::map<unsigned int, DWORD> FrameToCSMap;
	FrameToCSMap _frame2cs;
#endif

	void SendFrame();

	void OnListenerEvent();
	void OnDisconnect(Peer *who, int err);

	void BroadcastTextMessage(const std::string &msg);

	// remote functions
	void SvControl(Peer *from, int task, const Variant &arg);
	void SvTextMessage(Peer *from, int task, const Variant &arg);
	void SvPlayerReady(Peer *from, int task, const Variant &arg);
	void SvAddBot(Peer *from, int task, const Variant &arg);
	void SvPlayerInfo(Peer *from, int task, const Variant &arg);

public:
	TankServer(const GameInfo &info, const SafePtr<LobbyClient> &announcer);
	~TankServer(void);

	std::string GetStats() const;
};

///////////////////////////////////////////////////////////////////////////////
// end of file
