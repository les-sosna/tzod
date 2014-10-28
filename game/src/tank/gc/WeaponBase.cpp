#include "WeaponBase.h"
#include "Light.h"
#include "Vehicle.h"
#include "Macros.h"
#include "World.h"
#include "WorldEvents.h"

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
		obj->_stayTimeout = (float) _propTimeStay.GetIntValue() / 1000.0f;
	}
	else
	{
		_propTimeStay.SetIntValue(int(obj->GetStayTimeout() * 1000.0f + 0.5f));
	}
}


GC_Weapon::GC_Weapon(vec2d pos)
  : GC_Pickup(pos)
  , _stayTimeout(15.0f)
  , _rotatorWeap(_angle)
{
	SetRespawnTime(GetDefaultRespawnTime());
}

void GC_Weapon::Fire(World &world, bool fire)
{
	SetFlags(GC_FLAG_WEAPON_FIRING, fire);
}

AIPRIORITY GC_Weapon::GetPriority(World &world, const GC_Vehicle &veh) const
{
	if( veh.GetWeapon() )
	{
		if( veh.GetWeapon()->GetBooster() )
			return AIP_NOTREQUIRED;

		if( GetBooster() )
			return AIP_WEAPON_ADVANCED;
		else
			return AIP_NOTREQUIRED;
	}

	return AIP_WEAPON_NORMAL + (GetBooster() ? AIP_WEAPON_ADVANCED : AIP_NOTREQUIRED);
}

void GC_Weapon::OnAttached(World &world, GC_Vehicle &vehicle)
{
	if (GC_Weapon *weapon = vehicle.GetWeapon())
		weapon->Disappear(world);
	vehicle.SetWeapon(world, this);

	_vehicle = &vehicle;

	_rotatorWeap.reset(0, 0, TOWER_ROT_SPEED, TOWER_ROT_ACCEL, TOWER_ROT_SLOWDOWN);

	SetVisible(true);
	SetBlinking(false);
}

void GC_Weapon::Detach(World &world)
{
	_vehicle = nullptr;
	_detachedTime = world.GetTime();
	Fire(world, false);
	GC_Pickup::Detach(world);
}

void GC_Weapon::Disappear(World &world)
{
	if (_booster)
		_booster->Disappear(world); // this will set _booster to nullptr
	GC_Pickup::Disappear(world);
}

void GC_Weapon::ProcessRotate(World &world, float dt)
{
	assert(GetAttached());
	_rotatorWeap.process_dt(dt);
	const VehicleState &vs = GetVehicle()->_state;
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

	vec2d a(_angle);
	vec2d direction = Vec2dAddDirection(GetVehicle()->GetDirection(), a);
	SetDirection(direction);

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

void GC_Weapon::Serialize(World &world, SaveFile &f)
{
	GC_Pickup::Serialize(world, f);

	_rotatorWeap.Serialize(f);

	f.Serialize(_angle);
	f.Serialize(_detachedTime);
	f.Serialize(_stayTimeout);
	f.Serialize(_booster);
	f.Serialize(_vehicle);
}

void GC_Weapon::MoveTo(World &world, const vec2d &pos)
{
	if (_booster)
		_booster->MoveTo(world, pos);
	GC_Pickup::MoveTo(world, pos);
}

void GC_Weapon::Kill(World &world)
{
	if (_booster)
	{
		_booster->Disappear(world);
		assert(!_booster);
	}
	if( GetAttached() )
		Detach(world);
	GC_Pickup::Kill(world);
}

void GC_Weapon::TimeStep(World &world, float dt)
{
	GC_Pickup::TimeStep(world, dt);

	if( GetAttached() )
	{
		ProcessRotate(world, dt);
	}
	else
	{
		if( GetRespawn() && GetVisible() )
		{
			if( world.GetTime() >= GetDetachedTime() + GetStayTimeout() )
			{
				Disappear(world);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////

GC_ProjectileBasedWeapon::GC_ProjectileBasedWeapon(vec2d pos)
	: GC_Weapon(pos)
	, _lastShotTime(-FLT_MAX)
	, _lastShotPos(0,0)
	, _numShots(0)
	, _firing(nullptr)
{
}

GC_ProjectileBasedWeapon::GC_ProjectileBasedWeapon(FromFile)
	: GC_Weapon(FromFile())
	, _firing(nullptr)
{
}

GC_ProjectileBasedWeapon::~GC_ProjectileBasedWeapon()
{
}

void GC_ProjectileBasedWeapon::Shoot(World &world)
{
	SAFE_CANCEL(_firing);
	if( _numShots >= GetSeriesLength() )
		_numShots = 0;
	for( auto ls: world.eGC_ProjectileBasedWeapon._listeners )
		ls->OnShoot(*this);
	OnShoot(world);
	++_numShots;
	_firing = world.Timeout(*this, _numShots < GetSeriesLength() ? GetSeriesReloadTime() : GetReloadTime());
	_fireLight->SetActive(true);
	_lastShotTime = world.GetTime();
}

void GC_ProjectileBasedWeapon::Resume(World &world)
{
	_firing = nullptr;
	if( _numShots >= GetSeriesLength() )
		_numShots = 0;
	if (GetFire() || (GetContinuousSeries() && _numShots > 0))
	{
		Shoot(world);
	}
}

void GC_ProjectileBasedWeapon::Fire(World &world, bool fire)
{
	GC_Weapon::Fire(world, fire);
	if (fire && !_firing)
	{
		Resume(world);
	}
}

void GC_ProjectileBasedWeapon::ResetSeries()
{
	_numShots = 0;
	_lastShotTime = -FLT_MAX;
}

void GC_ProjectileBasedWeapon::OnAttached(World &world, GC_Vehicle &vehicle)
{
	_fireLight = &world.New<GC_Light>(vec2d(0, 0), GC_Light::LIGHT_POINT);
	_fireLight->SetActive(false);
	GC_Weapon::OnAttached(world, vehicle);
}

void GC_ProjectileBasedWeapon::Detach(World &world)
{
	ResetSeries();
	SAFE_CANCEL(_firing);
	SAFE_KILL(world, _fireLight);
	GC_Weapon::Detach(world);
}

void GC_ProjectileBasedWeapon::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);
	f.Serialize(_lastShotPos);
	f.Serialize(_lastShotTime);
	f.Serialize(_numShots);
	f.Serialize(_fireLight);
	// TODO: f.Serialize(_firing);
}

void GC_ProjectileBasedWeapon::OnUpdateView(World &world)
{
	if( _fireLight->GetActive() )
	{
		float feTime = GetFireEffectTime();
		float time = world.GetTime() - GetLastShotTime();
		if( time < feTime )
		{
			float op = 1.0f - pow(time / feTime, 2);
			vec2d dir = GetDirection();
			_fireLight->MoveTo(world, GetPos() + vec2d(_lastShotPos * dir, _lastShotPos.x*dir.y - _lastShotPos.y*dir.x));
			_fireLight->SetIntensity(op);
		}
		else
		{
			_fireLight->SetActive(false);
		}
	}
}
