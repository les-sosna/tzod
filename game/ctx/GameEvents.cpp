#include "inc/ctx/GameEvents.h"
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


void GameEventsBroadcaster::OnMurder(GC_Player &victim, GC_Player *killer, MurderType murderType)
{
	for (auto ls: _listeners)
		ls->OnMurder(victim, killer, murderType);
}
