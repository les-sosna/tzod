#include "WeaponBase.h"
#include "Light.h"
#include "Sound.h"
#include "Vehicle.h"
#include "Macros.h"
#include "World.h"

#include "SaveFile.h"

#include <cfloat>


PropertySet* GC_Weapon::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Weapon::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTimeStay(ObjectProperty::TYPE_INTEGER, "stay_time")
{
	_propTimeStay.SetIntRange(0, 1000000);
}

int GC_Weapon::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 1;
}

ObjectProperty* GC_Weapon::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propTimeStay; break;
	}

	assert(false);
	return NULL;
}

void GC_Weapon::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_Weapon *obj = static_cast<GC_Weapon*>(GetObject());
	if( applyToObject )
	{
		obj->_timeStay = (float) _propTimeStay.GetIntValue() / 1000.0f;
	}
	else
	{
		_propTimeStay.SetIntValue(int(obj->GetStayTime() * 1000.0f + 0.5f));
	}
}


GC_Weapon::GC_Weapon(World &world)
  : GC_Pickup(world)
  , _time(0)
  , _fePos(0,0)
  , _feTime(1.0f)
  , _lastShotTimestamp(-FLT_MAX)
  , _timeStay(15.0f)
  , _rotatorWeap(_angle)
{
	SetRespawnTime(GetDefaultRespawnTime());
	SetAutoSwitch(false);
}

AIPRIORITY GC_Weapon::GetPriority(World &world, const GC_Vehicle &veh) const
{
	if( veh.GetWeapon() )
	{
		if( veh.GetWeapon()->GetAdvanced() )
			return AIP_NOTREQUIRED;

		if( GetAdvanced() )
			return AIP_WEAPON_ADVANCED;
		else
			return AIP_NOTREQUIRED;
	}

	return AIP_WEAPON_NORMAL + (GetAdvanced() ? AIP_WEAPON_ADVANCED : AIP_NOTREQUIRED);
}

void GC_Weapon::Attach(World &world, GC_Actor *actor)
{
	assert(dynamic_cast<GC_Vehicle*>(actor));

	GC_Pickup::Attach(world, actor);

	_rotateSound = new GC_Sound(world, SND_TowerRotate, GetPos());
    _rotateSound->Register(world);
    _rotateSound->SetMode(world, SMODE_STOP);
	_rotatorWeap.reset(0, 0, TOWER_ROT_SPEED, TOWER_ROT_ACCEL, TOWER_ROT_SLOWDOWN);

	SetVisible(true);
	SetBlinking(false);

	_fireLight = new GC_Light(world, GC_Light::LIGHT_POINT);
    _fireLight->Register(world);
	_fireLight->SetActive(false);
}

void GC_Weapon::Detach(World &world)
{
	Fire(world, false);
	SAFE_KILL(world, _rotateSound);
	SAFE_KILL(world, _fireLight);

	_time = 0;

	GC_Pickup::Detach(world);
}

void GC_Weapon::ProcessRotate(World &world, float dt)
{
	assert(GetCarrier());
	_rotatorWeap.process_dt(dt);
	const VehicleState &vs = static_cast<const GC_Vehicle*>(GetCarrier())->_state;
	if( vs._bExplicitTower )
	{
		_rotatorWeap.rotate_to( vs._fTowerAngle );
	}
	else
	{
		if( vs._bState_TowerCenter )
			_rotatorWeap.rotate_to( 0.0f );
		else if( vs._bState_TowerLeft )
			_rotatorWeap.rotate_left();
		else if( vs._bState_TowerRight )
			_rotatorWeap.rotate_right();
		else if( RS_GETTING_ANGLE != _rotatorWeap.GetState() )
			_rotatorWeap.stop();
	}
	_rotatorWeap.SetupSound(world, _rotateSound);

	vec2d a(_angle);
	vec2d direction = Vec2dAddDirection(static_cast<GC_Vehicle*>(GetCarrier())->GetDirection(), a);
	SetDirection(direction);
	if( _fireLight->GetActive() )
	{
		if( _time < _feTime )
		{
			float op = 1.0f - pow(_time / _feTime, 2);
			_fireLight->MoveTo(world, GetPos() + vec2d(_fePos * direction, _fePos.x*direction.y - _fePos.y*direction.x));
			_fireLight->SetIntensity(op);
		}
		else
		{
			_fireLight->SetActive(false);
		}
	}

	OnUpdateView(world);
}

GC_Weapon::GC_Weapon(FromFile)
  : GC_Pickup(FromFile())
  , _rotatorWeap(_angle)
{
}

GC_Weapon::~GC_Weapon()
{
}

bool GC_Weapon::IsReady(const World &world) const
{
	return GetCarrier() && world.GetTime() > _lastShotTimestamp + GetReloadTime();
}

void GC_Weapon::Serialize(World &world, SaveFile &f)
{
	GC_Pickup::Serialize(world, f);

	_rotatorWeap.Serialize(f);

	f.Serialize(_angle);
	f.Serialize(_fePos);
	f.Serialize(_feTime);
	f.Serialize(_time);
	f.Serialize(_timeStay);
	f.Serialize(_fireLight);
	f.Serialize(_rotateSound);
}

void GC_Weapon::Kill(World &world)
{
	if( GetCarrier() )
		Detach(world);
	assert(!_rotateSound);
	GC_Pickup::Kill(world);
}

void GC_Weapon::TimeStep(World &world, float dt)
{
	GC_Pickup::TimeStep(world, dt);

	_time += dt;

	if( GetCarrier() )
	{
		ProcessRotate(world, dt);
	}
	else
	{
		if( GetRespawn() && GetVisible() )
		{
			if( _time > GetStayTime() )
			{
				Disappear(world);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////

GC_ProjectileBasedWeapon::GC_ProjectileBasedWeapon(World &world)
	: GC_Weapon(world)
{
}

GC_ProjectileBasedWeapon::GC_ProjectileBasedWeapon(FromFile)
	: GC_Weapon(FromFile())
{
}

GC_ProjectileBasedWeapon::~GC_ProjectileBasedWeapon()
{
}

void GC_ProjectileBasedWeapon::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);
	_time = GetReloadTime();
}


void GC_ProjectileBasedWeapon::Shoot(World &world)
{
	_time = 0;
	_lastShotTimestamp = world.GetTime();
	_fireLight->SetActive(true);
	OnShoot(world);
}
