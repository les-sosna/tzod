// script.h

#pragma once

#include <functional>

struct lua_State;
struct GLFWwindow;
class GC_Object;
class World;
class ThemeManager;
class TextureManager;
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
	FS::FileSystem &fs;
	ThemeManager &themeManager;
	TextureManager &textureManager;
	std::function<void()> exitCommand;
	
#ifndef NOSOUND
	std::unique_ptr<MusicPlayer> music;
#endif
};

lua_State* script_open(ScriptEnvironment &se);
void       script_close(lua_State *L);

bool script_exec(lua_State *L, const char *string);
bool script_exec_file(lua_State *L, const char *filename);

// aux
int luaT_ConvertVehicleClass(lua_State *L);
void luaT_pushobject(lua_State *L, class GC_Object *obj);
void ClearCommandQueue(lua_State *L);
void RunCmdQueue(lua_State *L, float dt);

// end of file
