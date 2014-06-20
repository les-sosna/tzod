// Weapons.cpp

#include "Weapons.h"
#include "Vehicle.h"
#include "RigidBodyDinamic.h"
#include "Sound.h"
#include "Light.h"
#include "Player.h"
#include "Indicators.h"
#include "projectiles.h"
#include "Particles.h"

#include "Macros.h"
#include "World.h"

#include "SaveFile.h"

#include <cfloat>


IMPLEMENT_SELF_REGISTRATION(GC_FireWeapEffect)
{
	return true;
}

IMPLEMENT_SELF_REGISTRATION(GC_Crosshair)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////


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
		_propTimeStay.SetIntValue(int(obj->_timeStay * 1000.0f + 0.5f));
	}
}


GC_Weapon::GC_Weapon(World &world)
  : GC_Pickup(world)
  , _fePos(0,0)
  , _feOrient(1,0)
  , _feTime(1.0f)
  , _advanced(false)
  , _time(0)
  , _timeStay(15.0f)
  , _timeReload(0)
  , _rotatorWeap(_angle)
  , _fixmeChAnimate(true)
{
	SetRespawnTime(GetDefaultRespawnTime());
	SetAutoSwitch(false);
}

AIPRIORITY GC_Weapon::GetPriority(World &world, const GC_Vehicle &veh) const
{
	if( veh.GetWeapon() )
	{
		if( veh.GetWeapon()->_advanced )
			return AIP_NOTREQUIRED;

		if( _advanced )
			return AIP_WEAPON_ADVANCED;
		else
			return AIP_NOTREQUIRED;
	}

	return AIP_WEAPON_NORMAL + (_advanced ? AIP_WEAPON_ADVANCED : AIP_NOTREQUIRED);
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

	SetCrosshair(world);
	if( _crosshair )
	{
		if( GC_Vehicle *veh = dynamic_cast<GC_Vehicle*>(GetCarrier()) )
		{
			_crosshair->SetVisible(NULL != dynamic_cast<GC_Player*>(veh->GetOwner()));
		}
		else
		{
			_crosshair->SetVisible(false);
		}
	}


	PLAY(SND_w_Pickup, GetPos());

	_fireEffect = new GC_FireWeapEffect();
    _fireEffect->Register(world);
	_fireEffect->SetVisible(false);

	_fireLight = new GC_Light(world, GC_Light::LIGHT_POINT);
    _fireLight->Register(world);
	_fireLight->SetActive(false);
}

void GC_Weapon::Detach(World &world)
{
	SAFE_KILL(world, _rotateSound);
	SAFE_KILL(world, _crosshair);
	SAFE_KILL(world, _fireEffect);
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
	if( _fireEffect->GetVisible() )
	{
		int frame = int( _time / _feTime * (float) _fireEffect->GetFrameCount() );
		if( frame < _fireEffect->GetFrameCount() )
		{
			float op = 1.0f - pow(_time / _feTime, 2);

			_fireEffect->SetFrame(frame);
			_fireEffect->SetDirection(Vec2dAddDirection(direction, _feOrient));
			_fireEffect->SetOpacity(op);

			_fireEffect->MoveTo(world, GetPos() + vec2d(_fePos * direction, _fePos.x*direction.y - _fePos.y*direction.x));
			_fireLight->MoveTo(world, _fireEffect->GetPos());
			_fireLight->SetIntensity(op);
			_fireLight->SetActive(true);
		}
		else
		{
			_fireEffect->SetFrame(0);
			_fireEffect->SetVisible(false);
			_fireLight->SetActive(false);
		}
	}

	OnUpdateView(world);
}

void GC_Weapon::SetCrosshair(World &world)
{
	_crosshair = new GC_Crosshair();
    _crosshair->Register(world);
	_crosshair->SetTexture("indicator_crosshair1");
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
	f.Serialize(_advanced);
	f.Serialize(_feOrient);
	f.Serialize(_fePos);
	f.Serialize(_feTime);
	f.Serialize(_time);
	f.Serialize(_timeStay);
	f.Serialize(_timeReload);
	f.Serialize(_crosshair);
	f.Serialize(_fireEffect);
	f.Serialize(_fireLight);
	f.Serialize(_rotateSound);
	f.Serialize(_fixmeChAnimate);
}

void GC_Weapon::Kill(World &world)
{
	if( GetCarrier() )
	{
		Detach(world);
	}
	assert(!_crosshair);
	assert(!_rotateSound);
	assert(!_fireEffect);

	GC_Pickup::Kill(world);
}

void GC_Weapon::TimeStepFixed(World &world, float dt)
{
	GC_Pickup::TimeStepFixed(world, dt);

	_time += dt;

	if( GetCarrier() )
	{
		ProcessRotate(world, dt);
		if( _crosshair && _fixmeChAnimate )
		{
			_crosshair->MoveTo(world, GetPos() + GetDirection() * CH_DISTANCE_NORMAL);
			_crosshair->SetDirection(vec2d(_time * 5));
		}
	}
	else
	{
		if( GetRespawn() && GetVisible() )
		{
			SetBlinking(_time > _timeStay - 3.0f);
			if( _time > _timeStay )
			{
				SetBlinking(false);
				Disappear(world);
			}
		}
	}
}

void GC_Weapon::TimeStepFloat(World &world, float dt)
{
	GC_Pickup::TimeStepFloat(world, dt);
	if( !GetCarrier() && !GetRespawn() )
	{
		SetDirection(vec2d(GetTimeAnimation()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_RocketLauncher)
{
	ED_ITEM("weap_rockets", "obj_weap_rockets", 4 );
	return true;
}

void GC_Weap_RocketLauncher::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_timeReload = 2.0f;
	_time       = _timeReload;

	_reloaded         = true;
	_firing           = false;
	_nshots           = 0;
	_nshots_total     = 6;
	_time_shot        = 0.13f;

	_fireEffect->SetTexture("particle_fire3");

//return;
//	veh->SetMaxHP(85);

//	veh->_ForvAccel = 300;
//	veh->_BackAccel = 200;
//	veh->_StopAccel = 500;

//	veh->_rotator.setl(3.5f, 10.0f, 30.0f);

//	veh->_MaxBackSpeed = 150;
//	veh->_MaxForvSpeed = 150;
}

void GC_Weap_RocketLauncher::Detach(World &world)
{
	_firing = false;
	GC_Weapon::Detach(world);
}

GC_Weap_RocketLauncher::GC_Weap_RocketLauncher(World &world)
  : GC_Weapon(world)
  , _firing(false)
{
	_feTime = 0.1f;
	SetTexture("weap_ak47");
}

GC_Weap_RocketLauncher::GC_Weap_RocketLauncher(FromFile)
  : GC_Weapon(FromFile())
{
}

void GC_Weap_RocketLauncher::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);
	f.Serialize(_firing);
	f.Serialize(_reloaded);
	f.Serialize(_nshots);
	f.Serialize(_nshots_total);
	f.Serialize(_time_shot);
}

void GC_Weap_RocketLauncher::Fire(World &world)
{
	assert(GetCarrier());
	const vec2d &dir = GetDirection();
	if( _advanced )
	{
		if( _time >= _time_shot )
		{
			float dy = (((float)(world.net_rand()%(_nshots_total+1)) - 0.5f) / (float)_nshots_total - 0.5f) * 18.0f;
			_fePos.Set(13, dy);

			float ax = dir.x * 15.0f + dy * dir.y;
			float ay = dir.y * 15.0f - dy * dir.x;

			(new GC_Rocket(world, GetCarrier()->GetPos() + vec2d(ax, ay),
			               Vec2dAddDirection(dir, vec2d(world.net_frand(0.1f) - 0.05f)) * SPEED_ROCKET,
			               GetCarrier(), GetCarrier()->GetOwner(), _advanced))->Register(world);

			_time   = 0;
			_nshots = 0;
			_firing = false;

			_fireEffect->SetVisible(true);
		}
	}
	else
	{
		if( _firing )
		{
			if( _time >= _time_shot )
			{
				_nshots++;

				float dy = (((float)_nshots - 0.5f) / (float)_nshots_total - 0.5f) * 18.0f;
				_fePos.Set(13, dy);

				if( _nshots == _nshots_total )
				{
					_firing = false;
					_nshots = 0;
				}

				float ax = dir.x * 15.0f + dy * dir.y;
				float ay = dir.y * 15.0f - dy * dir.x;

				(new GC_Rocket(world, GetCarrier()->GetPos() + vec2d(ax, ay),
				               Vec2dAddDirection(dir, vec2d(world.net_frand(0.1f) - 0.05f)) * SPEED_ROCKET,
				               GetCarrier(), GetCarrier()->GetOwner(), _advanced))->Register(world);

				_time = 0;
				_fireEffect->SetVisible(true);
			}
		}

		if( _time >= _timeReload )
		{
			_firing = true;
			_time   = 0;
		}
	}

	_reloaded = false;
}

void GC_Weap_RocketLauncher::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.2f);
	pSettings->fProjectileSpeed   = SPEED_ROCKET;
	pSettings->fAttackRadius_max  = 600.0f;
	pSettings->fAttackRadius_min  = 100.0f;
	pSettings->fAttackRadius_crit =  40.0f;
	pSettings->fDistanceMultipler = _advanced ? 1.2f : 3.5f;
}

void GC_Weap_RocketLauncher::TimeStepFixed(World &world, float dt)
{
	if( GetCarrier() )
	{
		if( _firing )
			Fire(world);
		else if( _time >= _timeReload && !_reloaded )
		{
			_reloaded = true;
			if( !_advanced) PLAY(SND_WeapReload, GetPos());
		}
	}

	GC_Weapon::TimeStepFixed(world, dt);
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_AutoCannon)
{
	ED_ITEM( "weap_autocannon", "obj_weap_autocannon", 4 );
	return true;
}

GC_Weap_AutoCannon::GC_Weap_AutoCannon(World &world)
  : GC_Weapon(world)
{
	_feTime = 0.2f;
	SetTexture("weap_ac");
}

void GC_Weap_AutoCannon::SetAdvanced(World &world, bool advanced)
{
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(world, this, LOCATION_BOTTOM);
	if( pIndicator ) pIndicator->SetVisible(!advanced);
	if( _fireEffect ) _fireEffect->SetTexture(advanced ? "particle_fire4" : "particle_fire3");
	GC_Weapon::SetAdvanced(world, advanced);
}

void GC_Weap_AutoCannon::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_timeReload = 3.7f;
	_time       = _timeReload;

	_firing = false;
	_nshots = 0;
	_nshots_total = 30;
	_time_shot = 0.135f;

	GC_IndicatorBar *pIndicator = new GC_IndicatorBar(world, "indicator_ammo", this,
		(float *) &_nshots, (float *) &_nshots_total, LOCATION_BOTTOM);
    pIndicator->Register(world);
	pIndicator->SetInverse(true);

	_fireEffect->SetTexture("particle_fire3");

//return;
//	veh->SetMaxHP(80);

//	veh->_ForvAccel = 300;
//	veh->_BackAccel = 200;
//	veh->_StopAccel = 500;

//	veh->_rotator.setl(3.5f, 10.0f, 30.0f);

//	veh->_MaxForvSpeed = 240;
//	veh->_MaxBackSpeed = 160;
}

void GC_Weap_AutoCannon::Detach(World &world)
{
	GC_IndicatorBar *indicator = GC_IndicatorBar::FindIndicator(world, this, LOCATION_BOTTOM);
	if( indicator ) indicator->Kill(world);

	// kill the reload sound
	FOREACH( world.GetList(LIST_sounds), GC_Sound, object )
	{
		if( GC_Sound_link::GetTypeStatic() == object->GetType() )
		{
			if( ((GC_Sound_link *) object)->CheckObject(this) )
			{
				object->Kill(world);
				break;
			}
		}
	}

	GC_Weapon::Detach(world);
}

GC_Weap_AutoCannon::GC_Weap_AutoCannon(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_AutoCannon::~GC_Weap_AutoCannon()
{
}

void GC_Weap_AutoCannon::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);
	f.Serialize(_firing);
	f.Serialize(_nshots);
	f.Serialize(_nshots_total);
	f.Serialize(_time_shot);
}

void GC_Weap_AutoCannon::Fire(World &world)
{
	if( _firing && GetCarrier() )
	{
		const vec2d &dir = GetDirection();
		if( _advanced )
		{
			if( _time >= _time_shot )
			{
				for( int t = 0; t < 2; ++t )
				{
					float dy = t == 0 ? -9.0f : 9.0f;

					float ax = dir.x * 17.0f - dy * dir.y;
					float ay = dir.y * 17.0f + dy * dir.x;

					(new GC_ACBullet(world, GetCarrier()->GetPos() + vec2d(ax, ay),
									 dir * SPEED_ACBULLET,
									 GetCarrier(), GetCarrier()->GetOwner(), _advanced))->Register(world);
				}

				_time = 0;
				_fePos.Set(17.0f, 0);
				_fireEffect->SetVisible(true);

				PLAY(SND_ACShoot, GetPos());
			}
		}
		else
		{
			if( _time >= _time_shot )
			{
				_nshots++;

				float dy = (_nshots & 1) == 0 ? -9.0f : 9.0f;

				if( _nshots == _nshots_total )
				{
					_firing = false;
					auto sound = new GC_Sound_link(world, SND_AC_Reload, this);
                    sound->Register(world);
                    sound->SetMode(world, SMODE_PLAY);
				}

				float ax = dir.x * 17.0f - dy * dir.y;
				float ay = dir.y * 17.0f + dy * dir.x;

				(new GC_ACBullet(world, GetCarrier()->GetPos() + vec2d(ax, ay),
								 Vec2dAddDirection(dir, vec2d(world.net_frand(0.02f) - 0.01f)) * SPEED_ACBULLET,
								 GetCarrier(), GetCarrier()->GetOwner(), _advanced))->Register(world);

				_time = 0;
				_fePos.Set(17.0f, -dy);
				_fireEffect->SetVisible(true);

				PLAY(SND_ACShoot, GetPos());
			}
		}
	}
}

void GC_Weap_AutoCannon::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.1f);
	pSettings->fProjectileSpeed   = SPEED_ACBULLET;
	pSettings->fAttackRadius_max  = 500;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = _advanced ? 3.3f : 13.0f;
}

void GC_Weap_AutoCannon::TimeStepFixed(World &world, float dt)
{
	if( GetCarrier() )
	{
		if( _advanced )
			_nshots  = 0;

		if( _time >= _timeReload && !_firing )
		{
			_firing = true;
			_nshots  = 0;
			_time    = 0;
		}

		_firing |= _advanced;
	}

	GC_Weapon::TimeStepFixed(world, dt);
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Cannon)
{
	ED_ITEM( "weap_cannon", "obj_weap_cannon", 4 );
	return true;
}

GC_Weap_Cannon::GC_Weap_Cannon(World &world)
  : GC_Weapon(world)
{
	_fePos.Set(21, 0);
	_feTime = 0.2f;
	SetTexture("weap_cannon");
}

void GC_Weap_Cannon::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_timeReload    = 0.9f;
	_time_smoke_dt = 0;
	_time_smoke    = 0;

	_fireEffect->SetTexture("particle_fire3");

//return;
//	veh->SetMaxHP(125);

//	veh->_ForvAccel = 250;
//	veh->_BackAccel = 150;
//	veh->_StopAccel = 500;

//	veh->_rotator.setl(3.5f, 10.0f, 30.0f);

//	veh->_MaxForvSpeed = 160;
//	veh->_MaxBackSpeed = 120;
}

GC_Weap_Cannon::GC_Weap_Cannon(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Cannon::~GC_Weap_Cannon()
{
}

void GC_Weap_Cannon::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);
	f.Serialize(_time_smoke);
	f.Serialize(_time_smoke_dt);
}

void GC_Weap_Cannon::Fire(World &world)
{
	if( GetCarrier() && _time >= _timeReload )
	{
		GC_Vehicle * const veh = static_cast<GC_Vehicle*>(GetCarrier());
		const vec2d &dir = GetDirection();

		(new GC_TankBullet(world, GetPos() + dir * 17.0f,
			dir * SPEED_TANKBULLET + world.net_vrand(50), veh, veh->GetOwner(), _advanced))->Register(world);

		if( !_advanced )
		{
			veh->ApplyImpulse( dir * (-80.0f) );
		}

		_time       = 0;
		_time_smoke = 0.3f;

		_fireEffect->SetVisible(true);
	}
}

void GC_Weap_Cannon::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.1f);
	pSettings->fProjectileSpeed   = SPEED_TANKBULLET;
	pSettings->fAttackRadius_max  = 500;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit = _advanced ? 64.0f : 0;
	pSettings->fDistanceMultipler = _advanced ? 2.0f : 8.0f;
}

void GC_Weap_Cannon::TimeStepFixed(World &world, float dt)
{
	static const TextureCache tex("particle_smoke");

	GC_Weapon::TimeStepFixed(world, dt);

	if( GetCarrier() && _time_smoke > 0 )
	{
		_time_smoke -= dt;
		_time_smoke_dt += dt;

		for( ;_time_smoke_dt > 0; _time_smoke_dt -= 0.025f )
		{
			vec2d a = Vec2dAddDirection(static_cast<GC_Vehicle*>(GetCarrier())->GetDirection(), vec2d(_angle));
			auto p = new GC_Particle(world, Z_PARTICLE, SPEED_SMOKE + a * 50.0f, tex, frand(0.3f) + 0.2f);
            p->Register(world);
            p->MoveTo(world, GetPos() + a * 26.0f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Plazma)
{
	ED_ITEM( "weap_plazma", "obj_weap_plazma", 4 );
	return true;
}

GC_Weap_Plazma::GC_Weap_Plazma(World &world)
  : GC_Weapon(world)
{
	SetTexture("weap_plazma");
	_fePos.Set(0, 0);
	_feTime = 0.2f;
}

GC_Weap_Plazma::GC_Weap_Plazma(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Plazma::~GC_Weap_Plazma()
{
}

void GC_Weap_Plazma::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_timeReload = 0.3f;
	_fireEffect->SetTexture("particle_plazma_fire");

//return;
//	veh->SetMaxHP(100);

//	veh->_ForvAccel = 300;
//	veh->_BackAccel = 200;
//	veh->_StopAccel = 500;

//	veh->_rotator.setl(3.5f, 10.0f, 30.0f);

//	veh->_MaxForvSpeed = 200;
//	veh->_MaxBackSpeed = 160;
}

void GC_Weap_Plazma::Fire(World &world)
{
	if( GetCarrier() && _time >= _timeReload )
	{
		const vec2d &a = GetDirection();
		(new GC_PlazmaClod(world, GetPos() + a * 15.0f,
			a * SPEED_PLAZMA + world.net_vrand(20),
			GetCarrier(), GetCarrier()->GetOwner(), _advanced))->Register(world);
		_time = 0;
		_fireEffect->SetVisible(true);
	}
}

void GC_Weap_Plazma::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.2f);
	pSettings->fProjectileSpeed   = SPEED_PLAZMA;
	pSettings->fAttackRadius_max  = 300;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit = 0;
	pSettings->fDistanceMultipler = _advanced ? 2.0f : 8.0f;  // fixme
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Gauss)
{
	ED_ITEM( "weap_gauss", "obj_weap_gauss", 4 );
	return true;
}

GC_Weap_Gauss::GC_Weap_Gauss(World &world)
  : GC_Weapon(world)
{
	SetTexture("weap_gauss");
	_feTime = 0.15f;
}

void GC_Weap_Gauss::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_timeReload = 1.3f;
	_fireEffect->SetTexture("particle_gaussfire");

//return;
//	veh->SetMaxHP(70);

//	veh->_ForvAccel = 350;
//	veh->_BackAccel = 250;
//	veh->_StopAccel = 700;

//	veh->_rotator.setl(3.5f, 15.0f, 30.0f);

//	veh->_MaxBackSpeed = 220;
//	veh->_MaxForvSpeed = 260;
}

GC_Weap_Gauss::GC_Weap_Gauss(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Gauss::~GC_Weap_Gauss()
{
}

void GC_Weap_Gauss::Fire(World &world)
{
	if( GetCarrier() && _time >= _timeReload )
	{
		const vec2d &dir = GetDirection();
		(new GC_GaussRay(world, vec2d(GetPos().x + dir.x + 5 * dir.y, GetPos().y + dir.y - 5 * dir.x),
			dir * SPEED_GAUSS, GetCarrier(), GetCarrier()->GetOwner(), _advanced))->Register(world);

		_time = 0;
		_fireEffect->SetVisible(true);
	}
}

void GC_Weap_Gauss::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = false;
	pSettings->fMaxAttackAngleCos = cos(0.01f);
	pSettings->fProjectileSpeed   = 0;
	pSettings->fAttackRadius_max  = 800;
	pSettings->fAttackRadius_min  = 400;
	pSettings->fAttackRadius_crit = 0;
	pSettings->fDistanceMultipler = _advanced ? 4.5f : 9.0f;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Ram)
{
	ED_ITEM( "weap_ram", "obj_weap_ram", 4 );
	return true;
}

GC_Weap_Ram::GC_Weap_Ram(World &world)
  : GC_Weapon(world)
  , _firingCounter(0)
{
	SetTexture("weap_ram");
}

void GC_Weap_Ram::SetAdvanced(World &world, bool advanced)
{
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(world, this, LOCATION_BOTTOM);
	if( pIndicator ) pIndicator->SetVisible(!advanced);

	if( GetCarrier() )
	{
		static_cast<GC_Vehicle*>(GetCarrier())->_percussion =
			advanced ? WEAP_RAM_PERCUSSION * 2 : WEAP_RAM_PERCUSSION;
	}

	GC_Weapon::SetAdvanced(world, advanced);
}

void GC_Weap_Ram::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_engineSound = new GC_Sound(world, SND_RamEngine, GetPos());
    _engineSound->Register(world);
    _engineSound->SetMode(world, SMODE_STOP);
	_engineLight = new GC_Light(world, GC_Light::LIGHT_POINT);
    _engineLight->Register(world);
	_engineLight->SetIntensity(1.0f);
	_engineLight->SetRadius(120);
	_engineLight->SetActive(false);


	_fuel_max  = _fuel = 1.0f;
	_fuel_consumption_rate = 0.2f;
	_fuel_recuperation_rate  = 0.1f;

	_firingCounter = 0;
	_bReady = true;

	(new GC_IndicatorBar(world, "indicator_fuel", this, (float *) &_fuel, (float *) &_fuel_max, LOCATION_BOTTOM))->Register(world);

//return;
//	veh->SetMaxHP(350);

//	veh->_ForvAccel = 250;
//	veh->_BackAccel = 250;
//	veh->_StopAccel = 500;

//	veh->_percussion = _advanced ? WEAP_RAM_PERCUSSION * 2 : WEAP_RAM_PERCUSSION;

//	veh->_rotator.setl(3.5f, 10.0f, 30.0f);

//	veh->_MaxBackSpeed = 160;
//	veh->_MaxForvSpeed = 160;
}

void GC_Weap_Ram::Detach(World &world)
{
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(world, this, LOCATION_BOTTOM);
	if( pIndicator ) pIndicator->Kill(world);

	SAFE_KILL(world, _engineSound);
	SAFE_KILL(world, _engineLight);

	GC_Weapon::Detach(world);
}

GC_Weap_Ram::GC_Weap_Ram(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Ram::~GC_Weap_Ram()
{
}

void GC_Weap_Ram::Kill(World &world)
{
	SAFE_KILL(world, _engineSound);
    GC_Weapon::Kill(world);
}

void GC_Weap_Ram::OnUpdateView(World &world)
{
	_engineLight->MoveTo(world, GetPos() - GetDirection() * 20);
}

void GC_Weap_Ram::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);
	/////////////////////////////////////
	f.Serialize(_firingCounter);
	f.Serialize(_bReady);
	f.Serialize(_fuel);
	f.Serialize(_fuel_max);
	f.Serialize(_fuel_consumption_rate);
	f.Serialize(_fuel_recuperation_rate);
	f.Serialize(_engineSound);
	f.Serialize(_engineLight);
}

void GC_Weap_Ram::Fire(World &world)
{
	assert(GetCarrier());
	if( _bReady )
	{
		_firingCounter = 2;
		if( GC_RigidBodyDynamic *owner = dynamic_cast<GC_RigidBodyDynamic *>(GetCarrier()) )
		{
			owner->ApplyForce(GetDirection() * 2000);
		}
	}
}

void GC_Weap_Ram::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = false;
	pSettings->fMaxAttackAngleCos = cos(0.3f);
	pSettings->fProjectileSpeed   = 0;
	pSettings->fAttackRadius_max  = 100;
	pSettings->fAttackRadius_min  = 0;
	pSettings->fAttackRadius_crit = 0;
	pSettings->fDistanceMultipler = _advanced ? 2.5f : 6.0f;
}

void GC_Weap_Ram::TimeStepFloat(World &world, float dt)
{
	static const TextureCache tex1("particle_fire2");
	static const TextureCache tex2("particle_yellow");
	static const TextureCache tex3("particle_fire");


	if( GetCarrier() && _firingCounter )
	{
		GC_Vehicle *veh = static_cast<GC_Vehicle *>(GetCarrier());
		vec2d v = veh->_lv;

		// primary
		{
			const vec2d &a = GetDirection();
			vec2d emitter = GetPos() - a * 20.0f;
			for( int i = 0; i < 29; ++i )
			{
				float time = frand(0.05f) + 0.02f;
				float t = frand(6.0f) - 3.0f;
				vec2d dx(-a.y * t, a.x * t);
				auto p = new GC_Particle(world, Z_PARTICLE, v - a * frand(800.0f) - dx / time, fabs(t) > 1.5 ? tex1 : tex2, time);
                p->Register(world);
                p->MoveTo(world, emitter + dx);
			}
		}


		// secondary
		for( float l = -1; l < 2; l += 2 )
		{
			vec2d a = Vec2dAddDirection(GetDirection(), vec2d(l * 0.15f));
			vec2d emitter = GetPos() - a * 15.0f + vec2d( -a.y, a.x) * l * 17.0f;
			for( int i = 0; i < 10; i++ )
			{
				float time = frand(0.05f) + 0.02f;
				float t = frand(2.5f) - 1.25f;
				vec2d dx(-a.y * t, a.x * t);
				auto p = new GC_Particle(world, Z_PARTICLE, v - a * frand(600.0f) - dx / time, tex3, time);
                p->Register(world);
                p->MoveTo(world, emitter + dx);
			}
		}
	}

	GC_Weapon::TimeStepFloat(world, dt);
}

void GC_Weap_Ram::TimeStepFixed(World &world, float dt)
{
	GC_Weapon::TimeStepFixed(world, dt);

	if( GetCarrier() )
	{
		assert(_engineSound);

		if( _advanced )
			_fuel = _fuel_max;

		if( _firingCounter )
		{
			_engineSound->Pause(world, false);
			_engineSound->MoveTo(world, GetPos());

			_fuel = std::max(.0f, _fuel - _fuel_consumption_rate * dt);
			if( 0 == _fuel ) _bReady = false;

			// the primary jet
			{
				const float lenght = 50.0f;
				const vec2d &a = GetDirection();
				vec2d emitter = GetPos() - a * 20.0f;
				vec2d hit;
				if( GC_RigidBodyStatic *object = world.TraceNearest(world.grid_rigid_s, GetCarrier(), emitter, -a * lenght, &hit) )
				{
					object->TakeDamage(world, dt * DAMAGE_RAM_ENGINE * (1.0f - (hit - emitter).len() / lenght), hit, GetCarrier()->GetOwner());
				}
			}

			// secondary jets
			for( float l = -1; l < 2; l += 2 )
			{
				const float lenght = 50.0f;
				vec2d a = Vec2dAddDirection(GetDirection(), vec2d(l * 0.15f));
				vec2d emitter = GetPos() - a * 15.0f + vec2d( -a.y, a.x) * l * 17.0f;
				vec2d hit;
				GC_RigidBodyStatic *object = world.TraceNearest(world.grid_rigid_s,
					GetCarrier(), emitter + a * 2.0f, -a * lenght, &hit);
				if( object )
				{
					object->TakeDamage(world,
						dt * DAMAGE_RAM_ENGINE * (1.0f - (hit - emitter).len() / lenght), hit, GetCarrier()->GetOwner());
				}
			}
		}
		else
		{
			_engineSound->Pause(world, true);
			_fuel   = std::min(_fuel_max, _fuel + _fuel_recuperation_rate * dt);
			_bReady = (_fuel_max < _fuel * 4.0f);
		}

		_engineLight->SetActive(_firingCounter > 0);
		if( _firingCounter ) --_firingCounter;
	}
	else
	{
		assert(!_engineSound);
	}

	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_BFG)
{
	ED_ITEM( "weap_bfg", "obj_weap_bfg", 4 );
	return true;
}

GC_Weap_BFG::GC_Weap_BFG(World &world)
  : GC_Weapon(world)
{
	SetTexture("weap_bfg");
}

void GC_Weap_BFG::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_time_ready  = 0;
	_timeReload = 1.1f;

//return;
//	veh->SetMaxHP(110);

//	veh->_ForvAccel = 250;
//	veh->_BackAccel = 200;
//	veh->_StopAccel = 1000;

//	veh->_rotator.setl(3.5f, 15.0f, 30.0f);

//	veh->_MaxForvSpeed = 200;
//	veh->_MaxBackSpeed = 180;
}

GC_Weap_BFG::GC_Weap_BFG(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_BFG::~GC_Weap_BFG()
{
}

void GC_Weap_BFG::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);
	f.Serialize(_time_ready);
}

void GC_Weap_BFG::Fire(World &world)
{
	assert(GetCarrier());

	if( _time >= _timeReload )
	{
		if( !_advanced && 0 == _time_ready )
		{
			PLAY(SND_BfgInit, GetPos());
			_time_ready = FLT_EPSILON;
		}

		if( _time_ready >= 0.7f || _advanced )
		{
			const vec2d &a = GetDirection();
			(new GC_BfgCore(world, GetPos() + a * 16.0f, a * SPEED_BFGCORE,
				GetCarrier(), GetCarrier()->GetOwner(), _advanced))->Register(world);
			_time_ready = 0;
			_time = 0;
		}
	}
}

void GC_Weap_BFG::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.01f);
	pSettings->fProjectileSpeed   = SPEED_BFGCORE;
	pSettings->fAttackRadius_max  = 600;
	pSettings->fAttackRadius_min  = 200;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = _advanced ? 13.0f : 20.0f;
}

void GC_Weap_BFG::TimeStepFixed(World &world, float dt)
{
	GC_Weapon::TimeStepFixed(world, dt);
	if( GetCarrier() && _time_ready != 0 )
	{
		_time_ready += dt;
		Fire(world);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_DiskSprite)
{
	return true;
}

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Ripper)
{
	ED_ITEM( "weap_ripper", "obj_weap_ripper", 4 );
	return true;
}

void GC_Weap_Ripper::UpdateDisk(World &world)
{
	_diskSprite->SetVisible(_time > _timeReload);
	_diskSprite->MoveTo(world, GetPos() - GetDirection() * 8);
	_diskSprite->SetDirection(vec2d(GetTimeAnimation() * 10));
}

void GC_Weap_Ripper::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_timeReload = 0.5f;
	_diskSprite = new GC_DiskSprite();
    _diskSprite->Register(world);
	_diskSprite->SetTexture("projectile_disk");
	UpdateDisk(world);

//return;
//	veh->SetMaxHP(80);

//	veh->_ForvAccel = 300;
//	veh->_BackAccel = 200;
//	veh->_StopAccel = 500;

//	veh->_rotator.setl(3.5f, 10.0f, 30.0f);

//	veh->_MaxBackSpeed = 260;
//	veh->_MaxForvSpeed = 240;
}

void GC_Weap_Ripper::Detach(World &world)
{
	SAFE_KILL(world, _diskSprite);
	GC_Weapon::Detach(world);
}

GC_Weap_Ripper::GC_Weap_Ripper(World &world)
  : GC_Weapon(world)
{
	SetTexture("weap_ripper");
}

GC_Weap_Ripper::GC_Weap_Ripper(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Ripper::~GC_Weap_Ripper()
{
}

void GC_Weap_Ripper::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);
	f.Serialize(_diskSprite);
}

void GC_Weap_Ripper::Fire(World &world)
{
	if( GetCarrier() && _time >= _timeReload )
	{
		const vec2d &a = GetDirection();
		(new GC_Disk(world, GetPos() - a * 9.0f, a * SPEED_DISK + world.net_vrand(10),
			GetCarrier(), GetCarrier()->GetOwner(), _advanced))->Register(world);
		PLAY(SND_DiskFire, GetPos());
		_time = 0;
	}
}

void GC_Weap_Ripper::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.2f);
	pSettings->fProjectileSpeed   = SPEED_DISK;
	pSettings->fAttackRadius_max  = 700;
	pSettings->fAttackRadius_min  = 500;
	pSettings->fAttackRadius_crit =  60;
	pSettings->fDistanceMultipler = _advanced ? 2.2f : 40.0f;
}

void GC_Weap_Ripper::TimeStepFloat(World &world, float dt)
{
	GC_Weapon::TimeStepFloat(world, dt);
	if( _diskSprite )
	{
		UpdateDisk(world);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Minigun)
{
	ED_ITEM( "weap_minigun", "obj_weap_minigun", 4 );
	return true;
}

GC_Weap_Minigun::GC_Weap_Minigun(World &world)
  : GC_Weapon(world)
  , _bFire(false)
{
	SetTexture("weap_mg1");

	_fePos.Set(20, 0);
	_feTime   = 0.1f;
	_feOrient = vrand(1);
}

GC_Weap_Minigun::GC_Weap_Minigun(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Minigun::~GC_Weap_Minigun()
{
}

void GC_Weap_Minigun::Kill(World &world)
{
	SAFE_KILL(world, _crosshairLeft);
	SAFE_KILL(world, _sound);
    GC_Weapon::Kill(world);
}

void GC_Weap_Minigun::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_timeReload = 0.03f;
	_timeRotate = 0;
	_timeFire   = 0;
	_timeShot   = 0;

	_sound = new GC_Sound(world, SND_MinigunFire, GetPos());
    _sound->Register(world);
    _sound->SetMode(world, SMODE_STOP);
	_bFire = false;

	_fireEffect->SetTexture("minigun_fire");


	if( _crosshairLeft )
	{
		if( GC_Vehicle *veh = dynamic_cast<GC_Vehicle*>(GetCarrier()) )
		{
			_crosshairLeft->SetVisible(NULL != dynamic_cast<GC_Player*>(veh->GetOwner()));
		}
		else
		{
			_crosshairLeft->SetVisible(false);
		}
	}



//return;
//	veh->SetMaxHP(65);

//	veh->_ForvAccel = 700;
//	veh->_BackAccel = 600;
//	veh->_StopAccel = 2000;

//	veh->_rotator.setl(3.5f, 15.0f, 30.0f);

//	veh->_MaxBackSpeed = 300;
//	veh->_MaxForvSpeed = 350;

}

void GC_Weap_Minigun::Detach(World &world)
{
	SAFE_KILL(world, _crosshairLeft);
	SAFE_KILL(world, _sound);

	GC_Weapon::Detach(world);
}

void GC_Weap_Minigun::SetCrosshair(World &world)
{
	_crosshair = new GC_Crosshair();
    _crosshair->Register(world);
	_crosshair->SetTexture("indicator_crosshair2");

	_crosshairLeft = new GC_Crosshair();
    _crosshairLeft->Register(world);
	_crosshairLeft->SetTexture("indicator_crosshair2");

	_fixmeChAnimate = false;
}

void GC_Weap_Minigun::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);

	f.Serialize(_bFire);
	f.Serialize(_timeFire);
	f.Serialize(_timeRotate);
	f.Serialize(_timeShot);
	f.Serialize(_crosshairLeft);
	f.Serialize(_sound);
}

void GC_Weap_Minigun::Fire(World &world)
{
	assert(GetCarrier());
	if( GetCarrier() )
		_bFire = true;
}

void GC_Weap_Minigun::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.3f);
	pSettings->fProjectileSpeed   = SPEED_BULLET;
	pSettings->fAttackRadius_max  = 200;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = _advanced ? 5.0f : 10.0f;
}

void GC_Weap_Minigun::TimeStepFixed(World &world, float dt)
{
	static const TextureCache tex("particle_1");

	if( GetCarrier() )
	{
		GC_RigidBodyDynamic *veh = dynamic_cast<GC_RigidBodyDynamic *>(GetCarrier());
		if( _bFire )
		{
			_timeRotate += dt;
			_timeShot   += dt;

			SetTexture((fmod(_timeRotate, 0.08f) < 0.04f) ? "weap_mg1":"weap_mg2");

			_sound->MoveTo(world, GetPos());
			_sound->Pause(world, false);
			_bFire = false;

			for(; _timeShot > 0; _timeShot -= _advanced ? 0.02f : 0.04f)
			{
				_time = frand(_feTime);
				_feOrient = vrand(1);
				_fireEffect->SetVisible(true);

				float da = _timeFire * 0.07f / WEAP_MG_TIME_RELAX;

				vec2d a = Vec2dAddDirection(GetDirection(), vec2d(world.net_frand(da * 2.0f) - da));
				a *= (1 - world.net_frand(0.2f));

				if( veh && !_advanced )
				{
					if( world.net_frand(WEAP_MG_TIME_RELAX * 5.0f) < _timeFire - WEAP_MG_TIME_RELAX * 0.2f )
					{
						float m = 3000;//veh->_inv_i; // FIXME
						veh->ApplyTorque(m * (world.net_frand(1.0f) - 0.5f));
					}
				}

				GC_Bullet *tmp = new GC_Bullet(world, GetPos() + a * 18.0f, a * SPEED_BULLET, GetCarrier(), GetCarrier()->GetOwner(), _advanced);
                tmp->Register(world);
				tmp->TimeStepFixed(world, _timeShot);
			}

			_timeFire = std::min(_timeFire + dt * 2, WEAP_MG_TIME_RELAX);
		}
		else
		{
			_sound->Pause(world, true);
			_timeFire = std::max(_timeFire - dt, .0f);
		}

		vec2d delta(_timeFire * 0.1f / WEAP_MG_TIME_RELAX);
		if( _crosshair )
		{
			_crosshair->SetDirection(Vec2dAddDirection(GetDirection(), delta));
			_crosshair->MoveTo(world, GetPos() + _crosshair->GetDirection() * CH_DISTANCE_THIN);
		}
		if( _crosshairLeft )
		{
			_crosshairLeft->SetDirection(Vec2dSubDirection(GetDirection(), delta));
			_crosshairLeft->MoveTo(world, GetPos() + _crosshairLeft->GetDirection() * CH_DISTANCE_THIN);
		}
	}

	GC_Weapon::TimeStepFixed(world, dt);
}


//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Zippo)
{
	ED_ITEM( "weap_zippo", "obj_weap_zippo", 4 );
	return true;
}

GC_Weap_Zippo::GC_Weap_Zippo(World &world)
  : GC_Weapon(world)
  , _timeBurn(0)
  , _bFire(false)
{
	SetTexture("weap_zippo");
}

GC_Weap_Zippo::GC_Weap_Zippo(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Zippo::~GC_Weap_Zippo()
{
}

void GC_Weap_Zippo::Kill(World &world)
{
	SAFE_KILL(world, _sound);
    GC_Weapon::Kill(world);
}

void GC_Weap_Zippo::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_timeReload = 0.02f;
	_timeFire   = 0;
	_timeShot   = 0;

	_sound = new GC_Sound(world, SND_RamEngine, GetPos());
    _sound->Register(world);
    _sound->SetMode(world, SMODE_STOP);
	_bFire = false;
}

void GC_Weap_Zippo::Detach(World &world)
{
	SAFE_KILL(world, _sound);
	GC_Weapon::Detach(world);
}

void GC_Weap_Zippo::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);

	f.Serialize(_bFire);
	f.Serialize(_timeFire);
	f.Serialize(_timeShot);
	f.Serialize(_timeBurn);
	f.Serialize(_sound);
}

void GC_Weap_Zippo::Fire(World &world)
{
	assert(GetCarrier());
	_bFire = true;
}

void GC_Weap_Zippo::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.5f);
	pSettings->fProjectileSpeed   = SPEED_FIRE;
	pSettings->fAttackRadius_max  = 300;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit =  10;
	pSettings->fDistanceMultipler = _advanced ? 5.0f : 10.0f;
}

void GC_Weap_Zippo::TimeStepFixed(World &world, float dt)
{
	GC_RigidBodyDynamic *veh = dynamic_cast<GC_RigidBodyDynamic *>(GetCarrier());

	if( GetCarrier() )
	{
		if( _bFire )
		{
			_timeShot += dt;
			_timeFire = std::min(_timeFire + dt, WEAP_ZIPPO_TIME_RELAX);

			_sound->MoveTo(world, GetPos());
			_sound->Pause(world, false);
			_bFire = false;


			vec2d vvel = veh ? veh->_lv : vec2d(0,0);

			for(; _timeShot > 0; _timeShot -= _timeReload )
			{
				vec2d a(GetDirection());
				a *= (1 - world.net_frand(0.2f));

				GC_FireSpark *tmp = new GC_FireSpark(world, GetPos() + a * 18.0f,
					vvel + a * SPEED_FIRE, GetCarrier(), GetCarrier()->GetOwner(), _advanced);
                tmp->Register(world);
				tmp->TimeStepFixed(world, _timeShot);
				tmp->SetLifeTime(_timeFire);
				tmp->SetHealOwner(_advanced);
				tmp->SetSetFire(true);
			}
		}
		else
		{
			_sound->Pause(world, true);
			_timeFire = std::max(_timeFire - dt, .0f);
		}
	}

	if( _advanced )
	{
		_timeBurn += dt;
		while( _timeBurn > 0 )
		{
			GC_FireSpark *tmp = new GC_FireSpark(world, GetPos() + world.net_vrand(33),
				SPEED_SMOKE/2, GetCarrier(), GetCarrier() ? GetCarrier()->GetOwner() : NULL, true);
            tmp->Register(world);
			tmp->SetLifeTime(0.3f);
			tmp->TimeStepFixed(world, _timeBurn);
			_timeBurn -= 0.01f;
		}
	}

	GC_Weapon::TimeStepFixed(world, dt);
}

///////////////////////////////////////////////////////////////////////////////
// end of file
