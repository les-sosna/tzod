#pragma once

class AppState;

class AppStateListener
{
public:
	explicit AppStateListener(AppState &appState);
	~AppStateListener();
	AppState& GetAppState() { return _appState; }
	const AppState& GetAppState() const { return _appState; }

	virtual void OnGameContextRemoving() = 0;
	virtual void OnGameContextRemoved() = 0;
	virtual void OnGameContextAdded() = 0;

private:
	AppState &_appState;

	AppStateListener(const AppStateListener &) = delete;
	void operator=(const AppStateListener &) = delete;
};
