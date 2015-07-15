#pragma once

struct lua_State;
class GC_Object;
class PropertySet;
class World;

World& luaT_getworld(lua_State *L);

void luaT_pushobject(lua_State *L, class GC_Object *obj);
GC_Object* luaT_checkobject(lua_State *L, int n);

// prop name at -2; prop value at -1; return 0 if property not found
int luaT_setproperty(lua_State *L, PropertySet &properties);
