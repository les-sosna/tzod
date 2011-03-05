// ClientBase.cpp

#include "stdafx.h"
#include "ClientBase.h"
#include "globals.h"

ClientBase::ClientBase()
{
	assert(!g_client);
	g_client = this;
}

ClientBase::~ClientBase()
{
	assert(_clientListeners.empty());
	assert(this == g_client);
	g_client = NULL;
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
