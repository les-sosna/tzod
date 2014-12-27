#include "AppState.h"
#include "GameContext.h"

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

AppStateListener::AppStateListener(AppState &appState)
: _appState(appState)
{
	_appState._appStateListeners.insert(this);
}

AppStateListener::~AppStateListener()
{
	_appState._appStateListeners.erase(this);
}
