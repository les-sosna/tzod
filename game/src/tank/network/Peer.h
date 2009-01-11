// Peer.h

#pragma once


#include "Socket.h"
#include "ControlPacket.h"

///////////////////////////////////////////////////////////////////////////////

// forward declarations
class DataBlock;


///////////////////////////////////////////////////////////////////////////////

class Peer : public RefCounted
{
public:
	Peer(SOCKET s);
	virtual ~Peer();

	void Close();
	void Connect(const sockaddr_in *addr);
	void Send(const DataBlock &db);
	Delegate<void(Peer *, const DataBlock &)> eventRecv;
	Delegate<void(Peer *, int errorCode)> eventDisconnect;
	Delegate<void(int errorCode)> eventConnect;

private:
	void OnSocketEvent();

	Socket _socket;
	std::vector<char> _outgoing;
	std::vector<char> _incoming;
};

// end of file
