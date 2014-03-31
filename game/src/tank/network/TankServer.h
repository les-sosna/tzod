// TankServer.h

#pragma once

#include "Peer.h"
#include "ControlPacket.h"
#include "CommonTypes.h"

#include "core/BitCounter.h"

#include "gc/Object.h" // for ObjPtr

class GC_PlayerHuman;


class PeerServer : public Peer
{
public:
	ObjPtr<GC_PlayerHuman> player;
	ControlPacket       ctrl;
	PlayerDesc          desc;
	int                 svlatency;
	float               clboost;
	BitCounter<128>     leading;
	bool                descValid;
	bool                ctrlValid;

	PeerServer(boost::asio::ip::tcp::socket s_);
};

///////////////////////////////////////////////////////////////////////////////

class LobbyClient;

class TankServer
{
	GameInfo _gameInfo;

	typedef std::list<SafePtr<PeerServer> >  PeerList;
	PeerList _clients;

	int _connectedCount;
	int _frameReadyCount;     // how many clients have ctrl data in buffer


	boost::asio::ip::tcp::socket _socketListen;

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
	~TankServer();

	std::string GetStats() const;
};

///////////////////////////////////////////////////////////////////////////////
// end of file
