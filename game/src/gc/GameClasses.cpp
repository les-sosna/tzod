#include "TypeReg.h"
#include "inc/gc/GameClasses.h"
#include "inc/gc/Player.h"
#include "inc/gc/RigidBody.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include "inc/gc/SaveFile.h"
#include <MapFile.h>


IMPLEMENT_SELF_REGISTRATION(GC_Wood)
{
	ED_BLOCK( "wood", "obj_wood",  7 );
	return true;
}

GC_Wood::GC_Wood(vec2d pos)
  : GC_Actor(pos)
{
}

GC_Wood::GC_Wood(FromFile)
  : GC_Actor(FromFile())
{
}

GC_Wood::~GC_Wood()
{
}

void GC_Wood::Init(World &world)
{
	GC_Actor::Init(world);
	int tileIndex = world.GetTileIndex(GetPos());
	if (-1 != tileIndex)
	{
		world._woodTiles[tileIndex] = true;
	}
}

void GC_Wood::Kill(World &world)
{
	int tileIndex = world.GetTileIndex(GetPos());
	if (-1 != tileIndex)
	{
		world._woodTiles[tileIndex] = false;
	}
	GC_Actor::Kill(world);
}


static const int dx[8] = {   1,   1,   0,  -1,  -1,  -1,   0,   1 };
static const int dy[8] = {   0,   1,   1,   1,   0,  -1,  -1,  -1 };
static const int nf[8] = { 131,   2,  14,   8,  56,  32, 224, 128 };

int GC_Wood::GetNeighbors(const World &world) const
{
	int x0 = (int)std::floor(GetPos().x / WORLD_BLOCK_SIZE);
	int y0 = (int)std::floor(GetPos().y / WORLD_BLOCK_SIZE);

	int neighbors = 0;
	for (int i = 0; i < 8; i++)
	{
		auto x = x0 + dx[i];
		auto y = y0 + dy[i];
		auto tileIndex = world.GetTileIndex(x, y);
		if (tileIndex == -1 || world._woodTiles[tileIndex])
		{
			neighbors |= nf[i];
		}
	}

	return neighbors;
}

void GC_Wood::MoveTo(World &world, const vec2d &pos)
{
	int oldTile = world.GetTileIndex(GetPos());
	int newTile = world.GetTileIndex(pos);
	if (oldTile != newTile)
	{
		if (-1 != oldTile)
			world._woodTiles[oldTile] = false;
		if (-1 != newTile)
			world._woodTiles[newTile] = true;
	}
	GC_Actor::MoveTo(world, pos);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_HealthDaemon)
{
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_HealthDaemon, LIST_timestep);

GC_HealthDaemon::GC_HealthDaemon(vec2d pos, GC_Player *owner, float damage, float time)
  : GC_Actor(pos)
  , _time(time)
  , _damage(damage)
  , _victim(nullptr)
  , _owner(owner)
{
}

GC_HealthDaemon::GC_HealthDaemon(FromFile)
  : GC_Actor(FromFile())
{
}

GC_HealthDaemon::~GC_HealthDaemon()
{
}

void GC_HealthDaemon::SetVictim(World &world, GC_RigidBodyStatic *victim)
{
	assert(!_victim && victim);
    _victim = victim;
	MoveTo(world, _victim->GetPos());
}

void GC_HealthDaemon::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);

	f.Serialize(_time);
	f.Serialize(_damage);
	f.Serialize(_victim);
	f.Serialize(_owner);
}

void GC_HealthDaemon::TimeStep(World &world, float dt)
{
	if (_victim)
	{
		// FIXME: depending on processing order this object
		//        may one time step lag behind
		MoveTo(world, _victim->GetPos());

		_time -= dt;
		bool kill = false;
		if( _time <= 0 )
		{
			dt += _time;
			kill = true;
		}
		DamageDesc dd;
		dd.damage = dt * _damage;
		dd.hit = _victim->GetPos();
		dd.from = _owner;
		_victim->TakeDamage(world, dd);
		if (kill)
			Kill(world);
	}
	else
	{
		Kill(world);
	}
}

/////////////////////////////////////////////////////////////

GC_Text::GC_Text(vec2d pos, std::string text)
  : GC_Actor(pos)
  , _style(DEFAULT)
  , _text(std::move(text))
{
}

GC_Text::~GC_Text()
{
}

void GC_Text::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);
	f.Serialize(_text);
	f.Serialize(_style);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Text_ToolTip)
{
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_Text_ToolTip, LIST_timestep);

GC_Text_ToolTip::GC_Text_ToolTip(vec2d pos, std::string text, Style style)
  : GC_Text(pos, std::move(text))
  , _time(0)
{
	SetStyle(style);
}

void GC_Text_ToolTip::Serialize(World &world, SaveFile &f)
{
	GC_Text::Serialize(world, f);
	f.Serialize(_time);
}

void GC_Text_ToolTip::TimeStep(World &world, float dt)
{
	MoveTo(world, GetPos() + vec2d{ 0, -20.0f } *dt);
	_time += dt;
	if( _time > 1.2f )
	{
		Kill(world);
	}
}
