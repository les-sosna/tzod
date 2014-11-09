#include "GameEvents.h"
#include <cassert>

GameEventsBroadcaster::~GameEventsBroadcaster()
{
	assert(_listeners.empty());
}

void GameEventsBroadcaster::AddListener(GameListener &ls)
{
	_listeners.insert(&ls);
}

void GameEventsBroadcaster::RemoveListener(GameListener &ls)
{
	assert(_listeners.count(&ls));
	_listeners.erase(&ls);
}

void GameEventsBroadcaster::OnGameMessage(const char *msg)
{
	for (auto ls: _listeners)
		ls->OnGameMessage(msg);
}
