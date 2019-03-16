#include "TypeReg.h"
#include "inc/gc/Macros.h"
#include "inc/gc/Player.h"
#include "inc/gc/SpawnPoint.h"
#include "inc/gc/Vehicle.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include "inc/gc/SaveFile.h"
#include <MapFile.h>

IMPLEMENT_SELF_REGISTRATION(GC_SpawnPoint)
{
	ED_MOVING_OBJECT("respawn_point", "obj_respawn",
	    0, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE/2, WORLD_BLOCK_SIZE/2);
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_MovingObject, GC_SpawnPoint, LIST_respawns);

GC_SpawnPoint::GC_SpawnPoint(vec2d pos)
  : GC_MovingObject(pos)
  , _team(0)
{
}

GC_SpawnPoint::GC_SpawnPoint(FromFile)
  : GC_MovingObject(FromFile())
{
}

void GC_SpawnPoint::Serialize(World &world, SaveFile &f)
{
	GC_MovingObject::Serialize(world, f);
	f.Serialize(_team);
}

void GC_SpawnPoint::MapExchange(MapFile &f)
{
	GC_MovingObject::MapExchange(f);

	float dir = GetDirection().Angle();

	MAP_EXCHANGE_FLOAT(dir, dir, 0);
	MAP_EXCHANGE_INT(team, _team, 0);

	if( f.loading() )
	{
		SetDirection(Vec2dDirection(dir));
		if( _team > MAX_TEAMS-1 )
			_team = MAX_TEAMS-1;
	}
}


PropertySet* GC_SpawnPoint::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_SpawnPoint::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTeam( ObjectProperty::TYPE_INTEGER, "team" )
  , _propDir( ObjectProperty::TYPE_FLOAT, "dir" )
{
	_propTeam.SetIntRange(0, MAX_TEAMS - 1);
	_propDir.SetFloatRange(0, PI2);
}

int GC_SpawnPoint::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_SpawnPoint::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propTeam;
	case 1: return &_propDir;
	}

	assert(false);
	return nullptr;
}

void GC_SpawnPoint::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_SpawnPoint *tmp = static_cast<GC_SpawnPoint *>(GetObject());

	if( applyToObject )
	{
		tmp->_team = _propTeam.GetIntValue();
		tmp->SetDirection(Vec2dDirection(_propDir.GetFloatValue()));
	}
	else
	{
		_propTeam.SetIntValue(tmp->_team);
		_propDir.SetFloatValue(tmp->GetDirection().Angle());
	}
}
