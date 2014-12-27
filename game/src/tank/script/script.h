// script.h

#pragma once

struct ScriptEnvironment;
struct lua_State;
namespace FS
{
	class FileSystem;
}

lua_State* script_open(ScriptEnvironment &se);

bool script_exec(lua_State *L, const char *string);
bool script_exec_file(lua_State *L, FS::FileSystem &fs, const char *filename);

// aux
int luaT_ConvertVehicleClass(lua_State *L);
void ClearCommandQueue(lua_State *L);
void RunCmdQueue(lua_State *L, float dt);

// end of file
