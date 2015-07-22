#include "inc/app/AppState.h"
#include "inc/app/AppStateListener.h"

AppStateListener::AppStateListener(AppState &appState)
	: _appState(appState)
{
	_appState._appStateListeners.insert(this);
}

AppStateListener::~AppStateListener()
{
	_appState._appStateListeners.erase(this);
}
