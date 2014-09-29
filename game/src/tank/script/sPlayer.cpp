#include "sPlayer.h"
#include "script.h"
#include "gc/Player.h"
#include "gc/Vehicle.h"
#include "gc/World.h"

sPlayer::sPlayer(World &world, lua_State *L)
	: _world(world)
	, _L(L)
{
	_world.eGC_Player.AddListener(*this);
}

sPlayer::~sPlayer()
{
	_world.eGC_Player.RemoveListener(*this);
}

void sPlayer::OnRespawn(GC_Player &obj, GC_Vehicle &vehicle)
{
	if( !obj.GetOnRespawn().empty() )
	{
		script_exec(_L, obj.GetOnRespawn().c_str());
	}
}

void sPlayer::OnDie(GC_Player &obj)
{
	if( !obj.GetOnDie().empty() )
	{
		script_exec(_L, obj.GetOnDie().c_str());
	}
}
