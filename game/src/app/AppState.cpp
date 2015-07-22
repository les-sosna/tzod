#include "inc/app/AppState.h"
#include "inc/app/AppStateListener.h"
#include "inc/app/GameContext.h"

AppState::AppState()
{
}

AppState::~AppState()
{
}

void AppState::SetGameContext(std::unique_ptr<GameContextBase> gameContext)
{
	for (AppStateListener *ls: _appStateListeners)
		ls->OnGameContextChanging();
	_gameContext = std::move(gameContext);
	for (AppStateListener *ls: _appStateListeners)
		ls->OnGameContextChanged();
}
