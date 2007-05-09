// script.h

#pragma once

///////////////////////////////////////////////////////////////////////////////
// script functions

typedef lua_State* script_h; // handle to the script engine
#define LS(s) (reinterpret_cast<lua_State*>(s))


script_h script_open(void);
void     script_close(script_h s);

bool script_exec(script_h s, const char *string);
bool script_exec_file(script_h s, const char *filename);

// aux
int luaT_ConvertVehicleClass(lua_State *L);


///////////////////////////////////////////////////////////////////////////////
// end of file
