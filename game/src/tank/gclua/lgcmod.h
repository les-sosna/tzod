#pragma once

struct lua_State;
class World;

struct ScriptEnvironment
{
	World &world;
};

ScriptEnvironment& GetScriptEnvironment(lua_State *L);
