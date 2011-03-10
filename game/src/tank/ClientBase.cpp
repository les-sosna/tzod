// ClientBase.cpp

#include "stdafx.h"
#include "ClientBase.h"
#include "globals.h"
#include "LevelInterfaces.h"
#include "ui/gui_desktop.h"
#include "ui/GuiManager.h"
#include "script.h"

ClientBase::ClientBase(ILevelController *level)
	: m_level(level)
{
	assert(!g_client);
	g_client = this;
}

ClientBase::~ClientBase()
{
	assert(_clientListeners.empty());
	assert(this == g_client);
	g_client = NULL;
	// remove all game objects
	m_level->Clear();
	// clear message area
	if( g_gui )
		static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->Clear();
	// cancel any pending commands
	ClearCommandQueue(g_env.L);
}

std::auto_ptr<Subscribtion> ClientBase::AddListener(IClientCallback *ls)
{
	return std::auto_ptr<Subscribtion>(new MySubscribtion(this, ls));
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
