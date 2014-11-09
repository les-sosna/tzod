#pragma once

#include <functional>

struct lua_State;
struct GameListener;
class TextureManager;
class ThemeManager;
class World;
namespace FS
{
	class FileSystem;
}

#ifndef NOSOUND
#include "sound/MusicPlayer.h"
#include <memory>
#endif

struct ScriptEnvironment
{
	World &world;
	GameListener &gameListener;
	FS::FileSystem &fs;
	ThemeManager &themeManager;
	TextureManager &textureManager;
	std::function<void()> exitCommand;
	
#ifndef NOSOUND
	std::unique_ptr<MusicPlayer> music;
#endif
};

ScriptEnvironment& GetScriptEnvironment(lua_State *L);
