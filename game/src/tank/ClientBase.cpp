// ClientBase.cpp

#include "ClientBase.h"
#include "globals.h"
#include "LevelInterfaces.h"
#include "ui/gui_desktop.h"
#include "ui/GuiManager.h"
#include "script.h"


ClientBase::ClientBase(ILevelController *level)
	: _level(level)
{
}

ClientBase::~ClientBase()
{
	assert(_clientListeners.empty());
	// remove all game objects
	_level->Clear();
	// clear message area
	if( g_gui )
		static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->Clear();
	// cancel any pending commands
	ClearCommandQueue(g_env.L);
}

std::unique_ptr<Subscribtion> ClientBase::AddListener(IClientCallback *ls)
{
	return std::unique_ptr<Subscribtion>(new MySubscribtion(this, ls));
}

///////////////////////////////////////////////////////////////////////////////

ClientBase::MySubscribtion::MySubscribtion(ClientBase *client, IClientCallback *callback)
	: _client(client)
	, _callback(callback)
{
	_client->_clientListeners.insert(_callback);
}

ClientBase::MySubscribtion::~MySubscribtion()
{
	_client->_clientListeners.erase(_callback);
}

// end of file
