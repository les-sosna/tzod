#pragma once
#include <memory>
#include <set>
#include <vector>

struct GameContextBase;
class AppStateListener;

class AppState final
{
public:
	AppState();
	~AppState();

	const std::shared_ptr<GameContextBase>& GetGameContext() const
	{
		static std::shared_ptr<GameContextBase> s_null;
		return _gameContexts.empty() ? s_null : _gameContexts.back();
	}

	void PushGameContext(std::shared_ptr<GameContextBase> gameContext);
	void PopGameContext();

private:
	std::vector<std::shared_ptr<GameContextBase>> _gameContexts;
	std::set<AppStateListener*> _appStateListeners;
	friend class AppStateListener;

	AppState(const AppState&) = delete;
	void operator=(const AppState&) = delete;
};
