#include "inc/as/AppState.h"
#include "inc/as/AppStateListener.h"
#include <ctx/GameContext.h>
#include <cassert>

AppState::AppState()
{
}

AppState::~AppState()
{
}

void AppState::PushGameContext(std::shared_ptr<GameContextBase> gameContext)
{
	assert(GetGameContext() != gameContext);
	_gameContexts.push_back(std::move(gameContext));
	for (AppStateListener* ls : _appStateListeners)
		ls->OnGameContextAdded();
}

void AppState::PopGameContext()
{
	assert(!_gameContexts.empty());
	for (AppStateListener* ls : _appStateListeners)
		ls->OnGameContextRemoving();
	_gameContexts.pop_back();
}
