#pragma once
#include "GameEvents.h"
#include "script/ScriptHarness.h"
#include "gclua/lgcmod.h"
#include <functional>
#include <string>
#include <vector>

class World;

struct GameContextBase
{
	virtual ~GameContextBase() {}
	virtual World& GetWorld() = 0;
	virtual void Step(float dt) = 0;
};


namespace FS
{
	class FileSystem;
	class Stream;
}
struct Gameplay;
class ThemeManager;
class TextureManager;
class WorldController;
class AIManager;
class InputManager;
#ifndef NOSOUND
class SoundHarness;
#endif

struct PlayerDesc
{
	std::string nick;
	std::string skin;
	std::string cls;
	unsigned int team;
};

struct DMSettings
{
	std::string mapName;
	std::vector<PlayerDesc> players;
	std::vector<PlayerDesc> bots;
};


class GameContext : public GameContextBase
{
public:
	GameContext(FS::FileSystem &fs, DMSettings settings);
	virtual ~GameContext();
	Gameplay& GetGameplay() { return *_gameplay; }
	WorldController& GetWorldController() { return *_worldController; }
	InputManager& GetInputManager() { return *_inputMgr; }
	GameEventSource& GetGameEventSource() { return _gameEventsBroadcaster; }
	
	void Serialize(FS::Stream &stream);
	void Deserialize(FS::Stream &stream);

	// GameContextBase
	virtual World& GetWorld() override { return *_world; }
	virtual void Step(float dt) override;

private:
	GameEventsBroadcaster _gameEventsBroadcaster;
	std::unique_ptr<World> _world;
	std::unique_ptr<Gameplay> _gameplay;
	std::unique_ptr<ScriptHarness> _scriptHarness;
	std::unique_ptr<WorldController> _worldController;
	std::unique_ptr<AIManager> _aiManager;
	std::unique_ptr<InputManager> _inputMgr;
};
