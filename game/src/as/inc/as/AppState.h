#pragma once
#include <memory>
#include <set>

struct GameContextBase;
class AppStateListener;

class AppState final
{
public:
	AppState();
	~AppState();
	const std::shared_ptr<GameContextBase>& GetGameContext() const { return _gameContext; }
	void SetGameContext(std::shared_ptr<GameContextBase> gameContext);

private:
	std::shared_ptr<GameContextBase> _gameContext;
	std::set<AppStateListener*> _appStateListeners;
	friend class AppStateListener;

	AppState(const AppState&) = delete;
	void operator=(const AppState&) = delete;
};
