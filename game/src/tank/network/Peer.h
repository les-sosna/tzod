// Peer.h

#pragma once


#include "Socket.h"
#include "Variant.h"

/*
struct
{
	int       taskId;
	int       func;
	int       type;   // 0 - post; 1 - send; 2 - reply
	Variant   arg;    // if type==0 || type==1 then arg else result
}
*/

///////////////////////////////////////////////////////////////////////////////

class Peer : public RefCounted
{
public:
	Peer(SOCKET s);
	virtual ~Peer();

	void Close();
	int Connect(const sockaddr_in *addr);
	Delegate<void(Peer *, int errorCode)> eventDisconnect;

	size_t GetSentRecent() { size_t tmp = _sentRecent; _sentRecent = 0; return tmp; }
	size_t GetPending() const { return _pendingCalls.size(); }
	size_t GetTrafficIn() const { return _in.GetTraffic(); }
	size_t GetTrafficOut() const { return _out.GetTraffic(); }

	void Post(int func, const Variant &arg);
	void Send(int func, const Variant &arg, Delegate<void(const Variant &)> onResult);

	void Reply(const Variant &result, int taskId);

	typedef Delegate<void(Peer *from, int taskId, const Variant &arg)> HandlerProc;
	void RegisterHandler(int func, Variant::TypeId argType, HandlerProc handler);

	void Pause();
	void Resume();

private:
	void OnSocketEvent();
	void ProcessInput();
	bool TrySend();

	Socket _socket;

	DataStream _in;
	DataStream _out;

	struct RemoteFunction
	{
		Variant::TypeId argType;
		HandlerProc handler;
	};

	typedef std::map<int, RemoteFunction> HandlersMap;
	HandlersMap _handlers;

	struct PendingRemoteCall
	{
		HandlerProc handler;
		Variant arg;
	};
	std::queue<PendingRemoteCall> _pendingCalls;

	size_t _sentRecent;
	bool _paused;
	bool _readyToSend;
};

// end of file
