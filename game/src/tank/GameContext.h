#pragma once
#include "Deathmatch.h"
#include "GameEvents.h"
#include "script/ScriptHarness.h"
#include "gc/World.h"
#include "gclua/lgcmod.h"
#include <functional>

namespace FS
{
	class FileSystem;
}
class ThemeManager;
class TextureManager;

class GameContext
{
public:
	GameContext(FS::FileSystem &fs,
				ThemeManager &themeManager,
				TextureManager &textureManager,
				std::function<void()> exitCommand);
	World& GetWorld() { return _world; }
	ScriptHarness& GetScriptHarness() { return _scriptHarness; }
	GameEventsBroadcaster& GetGameEventsBroadcaster() { return _gameEventsBroadcaster; }
	
	void Step(float dt);

private:	
	World _world;
	GameEventsBroadcaster _gameEventsBroadcaster;
	Deathmatch _deathmatch;
	ScriptEnvironment _scriptEnvironment;
	ScriptHarness _scriptHarness;
};
