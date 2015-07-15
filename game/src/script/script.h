// script.h

#pragma once

class World;
struct ScriptMessageSink;
struct lua_State;
namespace FS
{
	class FileSystem;
}

lua_State* script_open(World &world, ScriptMessageSink &messageSink);

void script_exec(lua_State *L, const char *string);
void script_exec_file(lua_State *L, FS::FileSystem &fs, const char *filename);

// aux
int luaT_ConvertVehicleClass(lua_State *L);
void ClearCommandQueue(lua_State *L);
void RunCmdQueue(lua_State *L, float dt, ScriptMessageSink &msgSink);

// end of file
