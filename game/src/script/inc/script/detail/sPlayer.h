#pragma once
#include <gc/WorldEvents.h>

class World;
struct lua_State;

class sPlayer : ObjectListener<GC_Player>
{
public:
	sPlayer(World &world, lua_State *L);
	~sPlayer();
	
private:
	World &_world;
	lua_State *_L;
	
	// ObjectListener<GC_Player>
	virtual void OnRespawn(GC_Player &obj, GC_Vehicle &vehicle) override;
	virtual void OnDie(GC_Player &obj) override;
};
