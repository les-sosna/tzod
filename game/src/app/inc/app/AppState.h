#pragma once
#include <memory>
#include <set>

struct GameContextBase;

class AppState
{
public:
	AppState();
	~AppState();
	GameContextBase* GetGameContext() const { return _gameContext.get(); }
	void SetGameContext(std::unique_ptr<GameContextBase> gameContext);
	
private:
	std::unique_ptr<GameContextBase> _gameContext;
	std::set<class AppStateListener*> _appStateListeners;
	friend class AppStateListener;
};

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
