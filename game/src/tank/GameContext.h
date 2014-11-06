#pragma once
#include "Deathmatch.h"
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
	
	void Step(float dt);

private:
	World _world;
	Deathmatch _deathmatch;
	ScriptEnvironment _scriptEnvironment;
	ScriptHarness _scriptHarness;
};
