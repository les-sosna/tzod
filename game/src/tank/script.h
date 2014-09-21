// script.h

#pragma once
#include "gclua/lgcmod.h"

struct lua_State;

lua_State* script_open(ScriptEnvironment &se);
void       script_close(lua_State *L);

bool script_exec(lua_State *L, const char *string);
bool script_exec_file(lua_State *L, const char *filename);

// aux
int luaT_ConvertVehicleClass(lua_State *L);
void ClearCommandQueue(lua_State *L);
void RunCmdQueue(lua_State *L, float dt);

// end of file
