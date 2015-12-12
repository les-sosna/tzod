#include "inc/as/AppState.h"
#include "inc/as/AppStateListener.h"

AppStateListener::AppStateListener(AppState &appState)
	: _appState(appState)
{
	_appState._appStateListeners.insert(this);
}

AppStateListener::~AppStateListener()
{
	_appState._appStateListeners.erase(this);
}
