// script.h

#pragma once

///////////////////////////////////////////////////////////////////////////////
// script functions


lua_State* script_open(void);
void       script_close(lua_State *L);

bool script_exec(lua_State *L, const char *string);
bool script_exec_file(lua_State *L, const char *filename);

// aux
int luaT_ConvertVehicleClass(lua_State *L);


///////////////////////////////////////////////////////////////////////////////
// end of file
