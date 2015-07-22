#pragma once

class AppState;

class AppStateListener
{
public:
	AppStateListener(AppState &appState);
	~AppStateListener();
	AppState& GetAppState() { return _appState; }

	virtual void OnGameContextChanging() = 0;
	virtual void OnGameContextChanged() = 0;

private:
	AppState &_appState;
};
