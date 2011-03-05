// ClientBase.h

#pragma once

#include "network/ControlPacket.h" // fixme: path

struct IClientCallback
{
	virtual void OnPlayersUpdate() {};
	virtual void OnStartGame() {};
	virtual void OnPlayerReady(size_t playerIdx, bool ready) {};
	virtual void OnTextMessage(const std::string &msg) {};
	virtual void OnErrorMessage(const std::string &msg) {};
	virtual void OnConnected() {};
	virtual void OnClientDestroy() = 0; // always implement this
};

class Subscribtion
{
public:
	virtual ~Subscribtion() = 0 {}
};

class ClientBase
{
public:
	ClientBase();
	virtual ~ClientBase() = 0;

	std::auto_ptr<Subscribtion> AddListener(IClientCallback *ls);

	virtual bool IsLocal() const = 0;
	virtual void SendControl(const ControlPacket &cp) = 0; // this function terminates current frame and starts next one
	virtual bool RecvControl(ControlPacketVector &result) = 0;

private:
	std::set<IClientCallback*> _clientListeners;
	class MySubscribtion : public Subscribtion
	{
	public:
		MySubscribtion(ClientBase *client, IClientCallback *callback);
		virtual ~MySubscribtion();
	private:
		ClientBase *_client;
		IClientCallback *_callback;
	};
};




// end of file
