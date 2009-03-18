// Peer.h

#pragma once


#include "Socket.h"
#include "ControlPacket.h"

/*
struct
{
	int       taskId;
	int       type;   // 0 - post; 1 - send; 2 - reply
	variant   arg;    // if type==0 || type==1 then arg else result
}


Peer
{
	void Post(int function, variant arg);               // no result
	void Send(int function, variant arg, Delegate<void(variant result)> cb);

	void Reply(variant result, int taskId);

	RegisterHandler(int function, Delegate<void(int taskId, variant arg)> cb);
}
*/

///////////////////////////////////////////////////////////////////////////////

// forward declarations
class DataBlock;


///////////////////////////////////////////////////////////////////////////////

class Variant;

class Peer : public RefCounted
{
public:
	Peer(SOCKET s);
	virtual ~Peer();

	void Close();
	int Connect(const sockaddr_in *addr);
	void Send(const DataBlock &db);
	Delegate<void(Peer *, const DataBlock &)> eventRecv;
	Delegate<void(Peer *, int errorCode)> eventDisconnect;


	void Post(int func, const Variant &arg);
	void Send(int func, const Variant &arg, Delegate<void(const Variant &)> onResult);

	void Reply(const Variant &result, int taskId);

	typedef Delegate<void(Peer *from, int taskId, const Variant &arg)> HandlerType;
	void RegisterHandler(int func, HandlerType handler);


private:
	void OnSocketEvent();

	Socket _socket;
	std::vector<char> _outgoing;
	std::vector<char> _incoming;

	std::map<int, HandlerType> _handlers;
};

// end of file
