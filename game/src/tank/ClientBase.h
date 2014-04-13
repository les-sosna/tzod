// ClientBase.h

#pragma once

#include "network/ControlPacket.h" // fixme: path
#include "gc/Object.h" // for ObjPtr
#include "Controller.h"

#include <memory>

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
	virtual ~Subscribtion() {}
};

struct PlayerHandle;

class ClientBase
{
public:
	explicit ClientBase(Level *level);
	virtual ~ClientBase() = 0;

	std::unique_ptr<Subscribtion> AddListener(IClientCallback *ls);

	virtual bool SupportPause() const = 0;
	virtual bool SupportEditor() const = 0;
	virtual bool SupportSave() const = 0;
	virtual bool IsLocal() const = 0;
	virtual void SendControl(const ControlPacket &cp) = 0; // this function terminates current frame and starts next one
	virtual bool RecvControl(std::vector<ControlPacket> &result) = 0;
    virtual const char* GetActiveProfile() const = 0;

protected:
	Level *_level;
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
