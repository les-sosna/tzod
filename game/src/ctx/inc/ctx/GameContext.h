#pragma once
#include "ScriptMessageBroadcaster.h"
#include "GameEvents.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class World;
struct Gameplay;

struct GameContextBase
{
	virtual ~GameContextBase() {}
	virtual World& GetWorld() = 0;
	virtual Gameplay* GetGameplay() = 0;
	virtual void Step(float dt) = 0;
};


namespace FS
{
	class FileSystem;
	struct Stream;
}

class AIManager;
class ScriptHarness;
class ThemeManager;
class TextureManager;
class WorldController;
class LangCache;

struct PlayerDesc
{
	std::string nick;
	std::string skin;
	std::string cls;
	unsigned int team;
};

struct DMSettings
{
	std::vector<PlayerDesc> players;
	std::vector<PlayerDesc> bots;
	int fragLimit = 0;
	float timeLimit = 0;
};


class GameContext : public GameContextBase
{
public:
	GameContext(FS::Stream &map, const DMSettings &settings);
	virtual ~GameContext();
	WorldController& GetWorldController() { return *_worldController; }
	GameEventSource& GetGameEventSource() { return _gameEventsBroadcaster; }
	ScriptMessageSource& GetScriptMessageSource() { return _scriptMessageBroadcaster; }

	void Serialize(FS::Stream &stream);
	void Deserialize(FS::Stream &stream);

	// GameContextBase
	World& GetWorld() override { return *_world; }
	Gameplay* GetGameplay() override;
	void Step(float dt) override;

private:
	GameEventsBroadcaster _gameEventsBroadcaster;
	app_detail::ScriptMessageBroadcaster _scriptMessageBroadcaster;
	std::unique_ptr<World> _world;
	std::unique_ptr<WorldController> _worldController;
	std::unique_ptr<Gameplay> _gameplay;
	std::unique_ptr<ScriptHarness> _scriptHarness;
	std::unique_ptr<AIManager> _aiManager;
};
