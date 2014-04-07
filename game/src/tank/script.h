// script.h

#pragma once

struct lua_State;
class GC_Object;

lua_State* script_open(void);
void       script_close(lua_State *L);

bool script_exec(lua_State *L, const char *string);
bool script_exec_file(lua_State *L, const char *filename);

// aux
int luaT_ConvertVehicleClass(lua_State *L);
void luaT_pushobject(lua_State *L, class GC_Object *obj);
void ClearCommandQueue(lua_State *L);

// end of file
