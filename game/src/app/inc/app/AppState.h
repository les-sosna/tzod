#pragma once
#include <memory>
#include <set>

struct GameContextBase;
class AppStateListener;

class AppState
{
public:
	AppState();
	~AppState();
	GameContextBase* GetGameContext() const { return _gameContext.get(); }
	void SetGameContext(std::unique_ptr<GameContextBase> gameContext);

private:
	std::unique_ptr<GameContextBase> _gameContext;
	std::set<AppStateListener*> _appStateListeners;
	friend class AppStateListener;

    AppState(const AppState&) = delete;
    void operator=(const AppState&) = delete;
};
