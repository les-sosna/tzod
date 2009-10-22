// Weapons.cpp

#include "stdafx.h"

#include "Weapons.h"

#include "Vehicle.h"
#include "Sound.h"
#include "Light.h"
#include "Player.h"
#include "Indicators.h"
#include "Projectiles.h"
#include "Particles.h"

#include "Macros.h"
#include "Level.h"
#include "functions.h"

#include "fs/SaveFile.h"

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

	assert(FALSE);
	return NULL;
}

void GC_Weapon::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

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


GC_Weapon::GC_Weapon(float x, float y)
  : GC_Pickup(x, y)
  , _rotatorWeap(_angleReal)
  , _advanced(false)
  , _feTime(1.0f)
  , _feOrient(0)
  , _fePos(0,0)
  , _time(0)
  , _timeStay(15.0f)
  , _timeReload(0)
{
	SetRespawnTime(GetDefaultRespawnTime());
	SetAutoSwitch(false);
}

AIPRIORITY GC_Weapon::GetPriority(GC_Vehicle *veh)
{
	if( veh->GetWeapon() )
	{
		if( veh->GetWeapon()->_advanced )
			return AIP_NOTREQUIRED;

		if( _advanced )
			return AIP_WEAPON_ADVANCED;
		else
			return AIP_NOTREQUIRED;
	}

	return AIP_WEAPON_NORMAL + _advanced ? AIP_WEAPON_ADVANCED : AIP_NOTREQUIRED;
}

void GC_Weapon::Attach(GC_Actor *actor)
{
	assert(dynamic_cast<GC_Vehicle*>(actor));
	GC_Vehicle *veh = static_cast<GC_Vehicle*>(actor);

	GC_Pickup::Attach(actor);


	SetZ(Z_ATTACHED_ITEM);

	_rotateSound = WrapRawPtr(new GC_Sound(SND_TowerRotate, SMODE_STOP, GetPos()));
	_rotatorWeap.reset(0, 0, TOWER_ROT_SPEED, TOWER_ROT_ACCEL, TOWER_ROT_SLOWDOWN);

	SetVisible(true);
	SetBlinking(false);

	SetCrosshair();
	if( _crosshair )
	{
		if( GC_Vehicle *veh = dynamic_cast<GC_Vehicle*>(GetOwner()) )
		{
			_crosshair->SetVisible(NULL != dynamic_cast<GC_PlayerLocal*>(veh->GetPlayer()));
		}
		else
		{
			_crosshair->SetVisible(false);
		}
	}


	PLAY(SND_w_Pickup, GetPos());

	_fireEffect = WrapRawPtr(new GC_UserSprite());
	_fireEffect->SetZ(Z_EXPLODE);
	_fireEffect->SetVisible(false);

	_fireLight = WrapRawPtr(new GC_Light(GC_Light::LIGHT_POINT));
	_fireLight->Activate(false);
}

void GC_Weapon::Detach()
{
	SAFE_KILL(_rotateSound);
	SAFE_KILL(_crosshair);
	SAFE_KILL(_fireEffect);
	SAFE_KILL(_fireLight);

	_time = 0;

	GC_Pickup::Detach();
}

void GC_Weapon::ProcessRotate(float dt)
{
	assert(IsAttached());
	_rotatorWeap.process_dt(dt);
	const VehicleState &vs = static_cast<GC_Vehicle*>(GetOwner())->_stateReal;
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
	_rotatorWeap.setup_sound(GetRawPtr(_rotateSound));

	vec2d a(_angleReal);
	_directionReal = Vec2dSumDirection(static_cast<GC_Vehicle*>(GetOwner())->GetDirection(), a);
	vec2d directionVisual = Vec2dSumDirection(static_cast<GC_Vehicle*>(GetOwner())->GetVisual()->GetDirection(), a);
	SetDirection(directionVisual);
	if( _fireEffect->GetVisible() )
	{
		int frame = int( _time / _feTime * (float) _fireEffect->GetFrameCount() );
		if( frame < _fireEffect->GetFrameCount() )
		{
			float op = 1.0f - pow(_time / _feTime, 2);

			_fireEffect->SetFrame(frame);
			_fireEffect->SetDirection(Vec2dSumDirection(directionVisual, vec2d((float) _feOrient)));
			_fireEffect->SetOpacity(op);

			_fireEffect->MoveTo(GetPosPredicted() + vec2d(_fePos * directionVisual, _fePos.x*directionVisual.y - _fePos.y*directionVisual.x));
			_fireLight->MoveTo(_fireEffect->GetPos());
			_fireLight->SetIntensity(op);
			_fireLight->Activate(true);
		}
		else
		{
			_fireEffect->SetFrame(0);
			_fireEffect->SetVisible(false);
			_fireLight->Activate(false);
		}
	}

	OnUpdateView();
}

void GC_Weapon::SetCrosshair()
{
	_crosshair = WrapRawPtr(new GC_Crosshair(GC_Crosshair::CHS_SINGLE));
}

GC_Weapon::GC_Weapon(FromFile)
  : GC_Pickup(FromFile())
  , _rotatorWeap(_angleReal)
{
}

GC_Weapon::~GC_Weapon()
{
}

void GC_Weapon::Serialize(SaveFile &f)
{
	GC_Pickup::Serialize(f);

	_rotatorWeap.Serialize(f);

	f.Serialize(_directionReal);
	f.Serialize(_angleReal);
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
}

void GC_Weapon::Kill()
{
	if( IsAttached() )
	{
		Detach();
	}
	assert(!_crosshair);
	assert(!_rotateSound);
	assert(!_fireEffect);

	GC_Pickup::Kill();
}

void GC_Weapon::TimeStepFixed(float dt)
{
	GC_Pickup::TimeStepFixed(dt);

	_time += dt;

	if( IsAttached() )
	{
		ProcessRotate(dt);
		if( _crosshair && GC_Crosshair::CHS_SINGLE == _crosshair->_chStyle )
		{
			_crosshair->MoveTo(GetPosPredicted() + GetDirection() * CH_DISTANCE_NORMAL);
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
				Disappear();
			}
		}
	}
}

void GC_Weapon::TimeStepFloat(float dt)
{
	GC_Pickup::TimeStepFloat(dt);
	if( !IsAttached() && !GetRespawn() )
	{
		SetSpriteRotation(GetTimeAnimation());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_RocketLauncher)
{
	ED_ITEM("weap_rockets", "obj_weap_rockets", 4 );
	return true;
}

void GC_Weap_RocketLauncher::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

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

void GC_Weap_RocketLauncher::Detach()
{
	_firing = false;
	GC_Weapon::Detach();
}

GC_Weap_RocketLauncher::GC_Weap_RocketLauncher(float x, float y)
  : GC_Weapon(x, y)
  , _firing(false)
{
	_feTime = 0.1f;
	SetTexture("weap_ak47");
}

GC_Weap_RocketLauncher::GC_Weap_RocketLauncher(FromFile)
  : GC_Weapon(FromFile())
{
}

void GC_Weap_RocketLauncher::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);
	f.Serialize(_firing);
	f.Serialize(_reloaded);
	f.Serialize(_nshots);
	f.Serialize(_nshots_total);
	f.Serialize(_time_shot);
}

void GC_Weap_RocketLauncher::Fire()
{
	assert(IsAttached());
	const vec2d &dir = GetDirectionReal();
	if( _advanced )
	{
		if( _time >= _time_shot )
		{
			float dy = (((float)(g_level->net_rand()%(_nshots_total+1)) - 0.5f) / (float)_nshots_total - 0.5f) * 18.0f;
			_fePos.Set(13, dy);

			float ax = dir.x * 15.0f + dy * dir.y;
			float ay = dir.y * 15.0f - dy * dir.x;

			new GC_Rocket(GetOwner()->GetPos() + vec2d(ax, ay),
			              Vec2dSumDirection(dir, vec2d(g_level->net_frand(0.1f) - 0.05f)) * SPEED_ROCKET,
			              static_cast<GC_Vehicle*>(GetOwner()), _advanced );

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

				new GC_Rocket( GetOwner()->GetPos() + vec2d(ax, ay),
				               Vec2dSumDirection(dir, vec2d(g_level->net_frand(0.1f) - 0.05f)) * SPEED_ROCKET,
				               static_cast<GC_Vehicle*>(GetOwner()), _advanced );

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
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngleCos = cos(0.2f);
	pSettings->fProjectileSpeed   = SPEED_ROCKET;
	pSettings->fAttackRadius_max  = 600.0f;
	pSettings->fAttackRadius_min  = 100.0f;
	pSettings->fAttackRadius_crit =  40.0f;
	pSettings->fDistanceMultipler = _advanced ? 1.2f : 3.5f;
}

void GC_Weap_RocketLauncher::TimeStepFixed(float dt)
{
	if( IsAttached() )
	{
		if( _firing )
			Fire();
		else if( _time >= _timeReload && !_reloaded )
		{
			_reloaded = true;
			if( !_advanced) PLAY(SND_WeapReload, GetPos());
		}
	}

	GC_Weapon::TimeStepFixed(dt);
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_AutoCannon)
{
	ED_ITEM( "weap_autocannon", "obj_weap_autocannon", 4 );
	return true;
}

GC_Weap_AutoCannon::GC_Weap_AutoCannon(float x, float y)
  : GC_Weapon(x, y)
{
	_feTime = 0.2f;
	SetTexture("weap_ac");
}

void GC_Weap_AutoCannon::SetAdvanced(bool advanced)
{
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(this, LOCATION_BOTTOM);
	if( pIndicator ) pIndicator->SetVisible(!advanced);
	if( _fireEffect ) _fireEffect->SetTexture(advanced ? "particle_fire4" : "particle_fire3");
	GC_Weapon::SetAdvanced(advanced);
}

void GC_Weap_AutoCannon::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

	_timeReload = 3.7f;
	_time       = _timeReload;

	_firing = false;
	_nshots = 0;
	_nshots_total = 30;
	_time_shot = 0.135f;

	GC_IndicatorBar *pIndicator = new GC_IndicatorBar("indicator_ammo", this,
		(float *) &_nshots, (float *) &_nshots_total, LOCATION_BOTTOM);
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

void GC_Weap_AutoCannon::Detach()
{
	GC_IndicatorBar *indicator = GC_IndicatorBar::FindIndicator(this, LOCATION_BOTTOM);
	if( indicator ) indicator->Kill();

	// убиваем звук перезар€дки
	FOREACH( g_level->GetList(LIST_sounds), GC_Sound, object )
	{
		if( GC_Sound_link::GetTypeStatic() == object->GetType() )
		{
			if( ((GC_Sound_link *) object)->CheckObject(this) )
			{
				object->Kill();
				break;
			}
		}
	}

	GC_Weapon::Detach();
}

GC_Weap_AutoCannon::GC_Weap_AutoCannon(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_AutoCannon::~GC_Weap_AutoCannon()
{
}

void GC_Weap_AutoCannon::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);
	f.Serialize(_firing);
	f.Serialize(_nshots);
	f.Serialize(_nshots_total);
	f.Serialize(_time_shot);
}

void GC_Weap_AutoCannon::Fire()
{
	if( _firing && IsAttached() )
	{
		float a = static_cast<GC_Vehicle*>(GetOwner())->GetSpriteRotation() + _angleReal;
		if( _advanced )
		{
			if( _time >= _time_shot )
			{
				for( int t = 0; t < 2; ++t )
				{
					float dy = t == 0 ? -9.0f : 9.0f;

					float ax = cosf(a) * 17.0f - dy * sinf(a);
					float ay = sinf(a) * 17.0f + dy * cosf(a);

					new GC_ACBullet(GetOwner()->GetPos() + vec2d(ax, ay),
									vec2d(a) * SPEED_ACBULLET,
									static_cast<GC_Vehicle*>(GetOwner()), _advanced );
				}

				_time   = 0;
				PLAY(SND_ACShoot, GetPos());

				_fePos.Set(17.0f, 0);
				_fireEffect->SetVisible(true);
			}
		}
		else
		{
			if( _time >= _time_shot )
			{
				_nshots++;

				float dang = g_level->net_frand(0.02f) - 0.01f;
				float dy = (_nshots & 1) == 0 ? -9.0f : 9.0f;

				if( _nshots == _nshots_total )
				{
					_firing = false;
					new GC_Sound_link(SND_AC_Reload, SMODE_PLAY, this);
				}

				float ax = cosf(a) * 17.0f - dy * sinf(a);
				float ay = sinf(a) * 17.0f + dy * cosf(a);

				new GC_ACBullet(GetOwner()->GetPos() + vec2d(ax, ay),
								vec2d(a + dang) * SPEED_ACBULLET,
								static_cast<GC_Vehicle*>(GetOwner()), _advanced );

				_time = 0;
				PLAY(SND_ACShoot, GetPos());

				_fePos.Set(17.0f, -dy);
				_fireEffect->SetVisible(true);
			}
		}
	}
}

void GC_Weap_AutoCannon::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngleCos = cos(0.1f);
	pSettings->fProjectileSpeed   = SPEED_ACBULLET;
	pSettings->fAttackRadius_max  = 500;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = _advanced ? 3.3f : 13.0f;
}

void GC_Weap_AutoCannon::TimeStepFixed(float dt)
{
	if( IsAttached() )
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

	GC_Weapon::TimeStepFixed(dt);
}

void GC_Weap_AutoCannon::Kill()
{
	GC_Weapon::Kill();
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Cannon)
{
	ED_ITEM( "weap_cannon", "obj_weap_cannon", 4 );
	return true;
}

GC_Weap_Cannon::GC_Weap_Cannon(float x, float y)
  : GC_Weapon(x, y)
{
	_fePos.Set(21, 0);
	_feTime = 0.2f;
	SetTexture("weap_cannon");
}

void GC_Weap_Cannon::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

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

void GC_Weap_Cannon::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);
	f.Serialize(_time_smoke);
	f.Serialize(_time_smoke_dt);
}

void GC_Weap_Cannon::Fire()
{
	if( IsAttached() && _time >= _timeReload )
	{
		GC_Vehicle * const veh = static_cast<GC_Vehicle*>(GetOwner());
		vec2d dir(veh->GetSpriteRotation() + _angleReal);

		new GC_TankBullet(GetPos() + dir * 17.0f, dir * SPEED_TANKBULLET + g_level->net_vrand(50),
			veh, _advanced );

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
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngleCos = cos(0.1f);
	pSettings->fProjectileSpeed   = SPEED_TANKBULLET;
	pSettings->fAttackRadius_max  = 500;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit = _advanced ? 64.0f : 0;
	pSettings->fDistanceMultipler = _advanced ? 2.0f : 8.0f;
}

void GC_Weap_Cannon::TimeStepFixed(float dt)
{
	static const TextureCache tex("particle_smoke");

	GC_Weapon::TimeStepFixed( dt );

	if( IsAttached() && _time_smoke > 0 )
	{
		_time_smoke -= dt;
		_time_smoke_dt += dt;

		for( ;_time_smoke_dt > 0; _time_smoke_dt -= 0.025f )
		{
			vec2d a = Vec2dSumDirection(static_cast<GC_Vehicle*>(GetOwner())->GetVisual()->GetDirection(), vec2d(_angleReal));
			new GC_Particle(GetPosPredicted() + a * 26.0f, SPEED_SMOKE + a * 50.0f, tex, frand(0.3f) + 0.2f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Plazma)
{
	ED_ITEM( "weap_plazma", "obj_weap_plazma", 4 );
	return true;
}

GC_Weap_Plazma::GC_Weap_Plazma(float x, float y)
  : GC_Weapon(x, y)
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

void GC_Weap_Plazma::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

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

void GC_Weap_Plazma::Fire()
{
	if( IsAttached() && _time >= _timeReload )
	{
		vec2d a(static_cast<GC_Vehicle*>(GetOwner())->GetSpriteRotation() + _angleReal);
		new GC_PlazmaClod(GetPos() + a * 15.0f,
			a * SPEED_PLAZMA + g_level->net_vrand(20),
			static_cast<GC_Vehicle*>(GetOwner()),
			_advanced );
		_time = 0;
		_fireEffect->SetVisible(true);
	}
}

void GC_Weap_Plazma::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
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

GC_Weap_Gauss::GC_Weap_Gauss(float x, float y)
  : GC_Weapon(x, y)
{
	SetTexture("weap_gauss");
	_feTime = 0.15f;
}

void GC_Weap_Gauss::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

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

void GC_Weap_Gauss::Fire()
{
	if( IsAttached() && _time >= _timeReload )
	{
		float a = static_cast<GC_Vehicle*>(GetOwner())->GetSpriteRotation() + _angleReal;
		float s = sinf(a);
		float c = cosf(a);

		new GC_GaussRay(vec2d(GetPos().x + c + 5 * s, GetPos().y + s - 5 * c),
			vec2d(c, s) * SPEED_GAUSS,
			static_cast<GC_Vehicle*>(GetOwner()), _advanced );

		_time = 0;
		_fireEffect->SetVisible(true);
	}
}

void GC_Weap_Gauss::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = FALSE;
	pSettings->fMaxAttackAngleCos = cos(0.01f);
	pSettings->fProjectileSpeed   = 0;
	pSettings->fAttackRadius_max  = 800;
	pSettings->fAttackRadius_min  = 400;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = _advanced ? 4.5f : 9.0f;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Ram)
{
	ED_ITEM( "weap_ram", "obj_weap_ram", 4 );
	return true;
}

GC_Weap_Ram::GC_Weap_Ram(float x, float y)
  : GC_Weapon(x, y)
  , _firingCounter(0)
{
	SetTexture("weap_ram");
}

void GC_Weap_Ram::SetAdvanced(bool advanced)
{
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(this, LOCATION_BOTTOM);
	if( pIndicator ) pIndicator->SetVisible(!advanced);

	if( GetOwner() )
	{
		static_cast<GC_Vehicle*>(GetOwner())->_percussion =
			advanced ? WEAP_RAM_PERCUSSION * 2 : WEAP_RAM_PERCUSSION;
	}

	GC_Weapon::SetAdvanced(advanced);
}

void GC_Weap_Ram::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

	_engineSound = WrapRawPtr(new GC_Sound(SND_RamEngine, SMODE_STOP, GetPos()));

	_engineLight = WrapRawPtr(new GC_Light(GC_Light::LIGHT_POINT));
	_engineLight->SetIntensity(1.0f);
	_engineLight->SetRadius(120);
	_engineLight->Activate(false);


	_fuel_max  = _fuel = 1.0f;
	_fuel_rate = 0.2f;
	_fuel_rep  = 0.1f;

	_firingCounter = 0;
	_bReady = true;

	GC_IndicatorBar *pIndicator = new GC_IndicatorBar("indicator_fuel", this,
		(float *) &_fuel, (float *) &_fuel_max, LOCATION_BOTTOM);

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

void GC_Weap_Ram::Detach()
{
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(this, LOCATION_BOTTOM);
	if( pIndicator ) pIndicator->Kill();

	SAFE_KILL(_engineSound);
	SAFE_KILL(_engineLight);

	GC_Weapon::Detach();
}

GC_Weap_Ram::GC_Weap_Ram(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Ram::~GC_Weap_Ram()
{
}

void GC_Weap_Ram::OnUpdateView()
{
	_engineLight->MoveTo(GetPosPredicted() - GetDirection() * 20);
}

void GC_Weap_Ram::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_firingCounter);
	f.Serialize(_bReady);
	f.Serialize(_fuel);
	f.Serialize(_fuel_max);
	f.Serialize(_fuel_rate);
	f.Serialize(_fuel_rep);
	f.Serialize(_engineSound);
	f.Serialize(_engineLight);
}

void GC_Weap_Ram::Kill()
{
	SAFE_KILL(_engineSound);
	GC_Weapon::Kill();
}

void GC_Weap_Ram::Fire()
{
	assert(IsAttached());

	if( _bReady )
	{
		_firingCounter = 2;
		if( GC_RigidBodyDynamic *owner = dynamic_cast<GC_RigidBodyDynamic *>(GetOwner()) )
		{
			owner->ApplyForce( vec2d(owner->GetSpriteRotation() + _angleReal) * 2000 );
		}
	}
}

void GC_Weap_Ram::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = FALSE;
	pSettings->fMaxAttackAngleCos = cos(0.3f);
	pSettings->fProjectileSpeed   = 0;
	pSettings->fAttackRadius_max  = 100;
	pSettings->fAttackRadius_min  = 0;
	pSettings->fAttackRadius_crit = 0;
	pSettings->fDistanceMultipler = _advanced ? 2.5f : 6.0f;
}

void GC_Weap_Ram::TimeStepFloat(float dt)
{
	static const TextureCache tex1("particle_fire2");
	static const TextureCache tex2("particle_yellow");
	static const TextureCache tex3("particle_fire");


	if( IsAttached() && _firingCounter )
	{
		GC_Vehicle *veh = static_cast<GC_Vehicle *>(GetOwner());
		vec2d v = veh->GetVisual()->_lv;

		// primary
		{
			const vec2d &a = GetDirection();
			vec2d emitter = GetPosPredicted() - a * 20.0f;
			for( int i = 0; i < 29; ++i )
			{
				float time = frand(0.05f) + 0.02f;
				float t = (frand(1.0f) - 0.5f) * 6.0f;
				vec2d dx( -a.y * t, a.x * t);
				new GC_Particle(emitter + dx, v - a * (frand(800.0f)) - dx / time, fabs(t) > 1.5 ? tex1 : tex2, time);
			}
		}


		// secondary
		for( float l = -1; l < 2; l += 2 )
		{
			vec2d a(veh->GetVisual()->GetSpriteRotation() + _angleReal + l * 0.15f);
			vec2d emitter = GetPosPredicted() - a * 15.0f + vec2d( -a.y, a.x) * l * 17.0f;
			for( int i = 0; i < 10; i++ )
			{
				float time = frand(0.05f) + 0.02f;
				float t = (frand(1.0f) - 0.5f) * 2.5f;
				vec2d dx( -a.y * t, a.x * t);
				new GC_Particle(emitter + dx, v - a * (frand(600.0f)) - dx / time, tex3, time);
			}
		}
	}

	GC_Weapon::TimeStepFloat(dt);
}

void GC_Weap_Ram::TimeStepFixed(float dt)
{
	GC_Weapon::TimeStepFixed( dt );

	if( IsAttached() )
	{
		assert(_engineSound);

		if( _advanced )
			_fuel = _fuel_max;

		if( _firingCounter )
		{
			_engineSound->Pause(false);
			_engineSound->MoveTo(GetPos());

			_fuel = __max(0, _fuel - _fuel_rate * dt);
			if( 0 == _fuel ) _bReady = false;

			GC_Vehicle *veh = static_cast<GC_Vehicle *>(GetOwner());

			vec2d v = veh->_lv;

			// основна€ стру€
			{
				const float lenght = 50.0f;
				vec2d a(veh->GetSpriteRotation() + _angleReal);
				vec2d emitter = GetPos() - a * 20.0f;
				vec2d hit;
				GC_RigidBodyStatic *object = g_level->agTrace (
					g_level->grid_rigid_s, veh, emitter, -a * lenght, &hit);
				if( object )
				{
					object->TakeDamage(
						dt * DAMAGE_RAM_ENGINE * (1.0f - (hit - emitter).len() / lenght), hit, veh);
				}
			}

			// боковые струи
			for( float l = -1; l < 2; l += 2 )
			{
				const float lenght = 50.0f;
				vec2d a(veh->GetSpriteRotation() + _angleReal + l * 0.15f);
				vec2d emitter = GetPos() - a * 15.0f + vec2d( -a.y, a.x) * l * 17.0f;
				vec2d hit;
				GC_RigidBodyStatic *object = g_level->agTrace(g_level->grid_rigid_s,
					veh, emitter + a * 2.0f, -a * lenght, &hit);
				if( object )
				{
					object->TakeDamage(
						dt * DAMAGE_RAM_ENGINE * (1.0f - (hit - emitter).len() / lenght), hit, veh);
				}
			}
		}
		else
		{
			_engineSound->Pause(true);
			_fuel   = __min(_fuel_max, _fuel + _fuel_rep * dt);
			_bReady = (_fuel_max < _fuel * 4.0f);
		}

		_engineLight->Activate(_firingCounter > 0);
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

GC_Weap_BFG::GC_Weap_BFG(float x, float y)
  : GC_Weapon(x, y)
{
	SetTexture("weap_bfg");
}

void GC_Weap_BFG::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

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

void GC_Weap_BFG::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);
	f.Serialize(_time_ready);
}

void GC_Weap_BFG::Fire()
{
	assert(IsAttached());

	if( _time >= _timeReload )
	{
		if( !_advanced && 0 == _time_ready )
		{
			PLAY(SND_BfgInit, GetPos());
			_time_ready = FLT_EPSILON;
		}

		if( _time_ready >= 0.7f || _advanced )
		{
			GC_Vehicle *veh = dynamic_cast<GC_Vehicle *>(GetOwner());
			vec2d a((veh ? veh->GetSpriteRotation() : 0) + _angleReal);

			new GC_BfgCore(GetPos() + a * 16.0f, a * SPEED_BFGCORE,
				dynamic_cast<GC_RigidBodyStatic*>(GetOwner()), _advanced );

			_time_ready = 0;
			_time = 0;
		}
	}
}

void GC_Weap_BFG::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngleCos = cos(0.01f);
	pSettings->fProjectileSpeed   = SPEED_BFGCORE;
	pSettings->fAttackRadius_max  = 600;
	pSettings->fAttackRadius_min  = 200;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = _advanced ? 13.0f : 20.0f;
}

void GC_Weap_BFG::TimeStepFixed(float dt)
{
	GC_Weapon::TimeStepFixed(dt);
	if( IsAttached() && _time_ready != 0 )
	{
		_time_ready += dt;
		Fire();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Ripper)
{
	ED_ITEM( "weap_ripper", "obj_weap_ripper", 4 );
	return true;
}

void GC_Weap_Ripper::UpdateDisk()
{
	_disk->SetVisible(_time > _timeReload);
	_disk->MoveTo(GetPosPredicted() - vec2d(GetSpriteRotation()) * 8);
}

void GC_Weap_Ripper::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

	_timeReload = 0.5f;
	_disk = WrapRawPtr(new GC_UserSprite());
	_disk->SetTexture("projectile_disk");
	_disk->SetZ(Z_PROJECTILE);
	UpdateDisk();

//return;
//	veh->SetMaxHP(80);

//	veh->_ForvAccel = 300;
//	veh->_BackAccel = 200;
//	veh->_StopAccel = 500;

//	veh->_rotator.setl(3.5f, 10.0f, 30.0f);

//	veh->_MaxBackSpeed = 260;
//	veh->_MaxForvSpeed = 240;
}

void GC_Weap_Ripper::Detach()
{
	SAFE_KILL(_disk);
	GC_Weapon::Detach();
}

GC_Weap_Ripper::GC_Weap_Ripper(float x, float y)
  : GC_Weapon(x, y)
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

void GC_Weap_Ripper::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);
	f.Serialize(_disk);
}

void GC_Weap_Ripper::Fire()
{
	if( IsAttached() && _time >= _timeReload )
	{
		GC_Vehicle *veh = dynamic_cast<GC_Vehicle *>(GetOwner());
		vec2d a((veh ? veh->GetSpriteRotation() : 0) + _angleReal);

		new GC_Disk(GetPos() - a * 9.0f, a * SPEED_DISK + g_level->net_vrand(10),
			dynamic_cast<GC_RigidBodyStatic*>(GetOwner()), _advanced );
		PLAY(SND_DiskFire, GetPos());

		_time = 0;
	}
}

void GC_Weap_Ripper::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngleCos = cos(0.2f);
	pSettings->fProjectileSpeed   = SPEED_DISK;
	pSettings->fAttackRadius_max  = 700;
	pSettings->fAttackRadius_min  = 500;
	pSettings->fAttackRadius_crit =  60;
	pSettings->fDistanceMultipler = _advanced ? 2.2f : 40.0f;
}

void GC_Weap_Ripper::TimeStepFloat(float dt)
{
	GC_Weapon::TimeStepFloat(dt);
	if( _disk )
	{
		_disk->SetSpriteRotation(_disk->GetSpriteRotation() + dt * 10);
		UpdateDisk();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Minigun)
{
	ED_ITEM( "weap_minigun", "obj_weap_minigun", 4 );
	return true;
}

GC_Weap_Minigun::GC_Weap_Minigun(float x, float y)
  : GC_Weapon(x, y)
  , _bFire(false)
{
	SetTexture("weap_mg1");

	_fePos.Set(20, 0);
	_feTime   = 0.1f;
	_feOrient = frand(PI2);
}

GC_Weap_Minigun::GC_Weap_Minigun(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Minigun::~GC_Weap_Minigun()
{
}

void GC_Weap_Minigun::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

	_timeReload = 0.03f;
	_timeRotate = 0;
	_timeFire   = 0;
	_timeShot   = 0;

	_sound = WrapRawPtr(new GC_Sound(SND_MinigunFire, SMODE_STOP, GetPos()));
	_bFire = false;

	_fireEffect->SetTexture("minigun_fire");


	if( _crosshairLeft )
	{
		if( GC_Vehicle *veh = dynamic_cast<GC_Vehicle*>(GetOwner()) )
		{
			_crosshairLeft->SetVisible(NULL != dynamic_cast<GC_PlayerLocal*>(veh->GetPlayer()));
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

void GC_Weap_Minigun::Detach()
{
	SAFE_KILL(_crosshairLeft);
	SAFE_KILL(_sound);

	GC_Weapon::Detach();
}

void GC_Weap_Minigun::SetCrosshair()
{
	_crosshair     = WrapRawPtr(new GC_Crosshair(GC_Crosshair::CHS_DOUBLE));
	_crosshairLeft = WrapRawPtr(new GC_Crosshair(GC_Crosshair::CHS_DOUBLE));
}

void GC_Weap_Minigun::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);

	f.Serialize(_bFire);
	f.Serialize(_timeFire);
	f.Serialize(_timeRotate);
	f.Serialize(_timeShot);
	f.Serialize(_crosshairLeft);
	f.Serialize(_sound);
}

void GC_Weap_Minigun::Kill()
{
	SAFE_KILL(_crosshairLeft);
	SAFE_KILL(_sound);

	GC_Weapon::Kill();
}

void GC_Weap_Minigun::Fire()
{
	assert(IsAttached());
	if( IsAttached() )
		_bFire = true;
}

void GC_Weap_Minigun::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngleCos = cos(0.3f);
	pSettings->fProjectileSpeed   = SPEED_BULLET;
	pSettings->fAttackRadius_max  = 200;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = _advanced ? 5.0f : 10.0f;
}

void GC_Weap_Minigun::TimeStepFixed(float dt)
{
	static const TextureCache tex("particle_1");

	if( IsAttached() )
	{
		GC_Vehicle *veh = dynamic_cast<GC_Vehicle *>(GetOwner());
		if( _bFire )
		{
			_timeRotate += dt;
			_timeShot   += dt;

			SetTexture((fmodf(_timeRotate, 0.08f) < 0.04f) ? "weap_mg1":"weap_mg2");

			_sound->MoveTo(GetPos());
			_sound->Pause(false);
			_bFire = false;

			for(; _timeShot > 0; _timeShot -= _advanced ? 0.02f : 0.04f)
			{
				_time = frand(_feTime);
				_feOrient = frand(PI2);
				_fireEffect->SetVisible(true);

				float da = _timeFire * 0.07f / WEAP_MG_TIME_RELAX;

				vec2d a(veh->GetSpriteRotation() + _angleReal + g_level->net_frand(da * 2.0f) - da);
				a *= (1 - g_level->net_frand(0.2f));

				if( veh && !_advanced )
				{
					if( g_level->net_frand(WEAP_MG_TIME_RELAX * 5.0f) < _timeFire - WEAP_MG_TIME_RELAX * 0.2f )
					{
						float m = 3000;//veh->_inv_i;
						veh->ApplyTorque(m * (g_level->net_frand(1.0f) - 0.5f));
					}
				}

				GC_Bullet *tmp = new GC_Bullet(GetPos() + a * 18.0f, a * SPEED_BULLET, veh, _advanced );
				tmp->TimeStepFixed(_timeShot);
			}

			_timeFire = __min(_timeFire + dt * 2, WEAP_MG_TIME_RELAX);
		}
		else
		{
			_sound->Pause(true);
			_timeFire = __max(_timeFire - dt, 0);
		}

		float va = veh ? veh->GetVisual()->GetSpriteRotation() : GetOwner()->GetSpriteRotation();
		float da = _timeFire * 0.1f / WEAP_MG_TIME_RELAX;
		if( _crosshair )
		{
			_crosshair->_angle = va + da + _angleReal;
			_crosshair->MoveTo(GetPosPredicted() + vec2d(_crosshair->_angle) * CH_DISTANCE_THIN);
		}
		if( _crosshairLeft )
		{
			_crosshairLeft->_angle = va - da + _angleReal;
			_crosshairLeft->MoveTo(GetPosPredicted() + vec2d(_crosshairLeft->_angle) * CH_DISTANCE_THIN);
		}
	}

	GC_Weapon::TimeStepFixed(dt);
}


//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Zippo)
{
	ED_ITEM( "weap_zippo", "obj_weap_zippo", 4 );
	return true;
}

GC_Weap_Zippo::GC_Weap_Zippo(float x, float y)
  : GC_Weapon(x, y)
  , _bFire(false)
  , _timeBurn(0)
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

void GC_Weap_Zippo::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

	_timeReload = 0.02f;
	_timeFire   = 0;
	_timeShot   = 0;

	_sound = WrapRawPtr(new GC_Sound(SND_RamEngine, SMODE_STOP, GetPos()));
	_bFire = false;
}

void GC_Weap_Zippo::Detach()
{
	SAFE_KILL(_sound);
	GC_Weapon::Detach();
}

void GC_Weap_Zippo::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);

	f.Serialize(_bFire);
	f.Serialize(_timeFire);
	f.Serialize(_timeShot);
	f.Serialize(_timeBurn);
	f.Serialize(_sound);
}

void GC_Weap_Zippo::Kill()
{
	SAFE_KILL(_sound);
	GC_Weapon::Kill();
}

void GC_Weap_Zippo::Fire()
{
	assert(IsAttached());
	_bFire = true;
}

void GC_Weap_Zippo::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngleCos = cos(0.5f);
	pSettings->fProjectileSpeed   = SPEED_FIRE;
	pSettings->fAttackRadius_max  = 300;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit =  10;
	pSettings->fDistanceMultipler = _advanced ? 5.0f : 10.0f;
}

void GC_Weap_Zippo::TimeStepFixed(float dt)
{
	GC_Vehicle *veh = dynamic_cast<GC_Vehicle *>(GetOwner());

	if( IsAttached() )
	{
		float va = veh ? veh->GetSpriteRotation() : 0;
		vec2d vvel = veh ? veh->_lv : vec2d(0,0);

		if( _bFire )
		{
			_timeShot += dt;
			_timeFire = __min(_timeFire + dt, WEAP_ZIPPO_TIME_RELAX);

			_sound->MoveTo(GetPos());
			_sound->Pause(false);
			_bFire = false;

			for(; _timeShot > 0; _timeShot -= _timeReload )
			{
				vec2d a(va + _angleReal);
				a *= (1 - g_level->net_frand(0.2f));

				GC_FireSpark *tmp = new GC_FireSpark(GetPos() + a * 18.0f, vvel/2 + a * SPEED_FIRE, veh, _advanced);
				tmp->TimeStepFixed(_timeShot);
				tmp->SetLifeTime(_timeFire);
				tmp->SetHealOwner(_advanced);
				tmp->SetSetFire(true);
			}
		}
		else
		{
			_sound->Pause(true);
			_timeFire = __max(_timeFire - dt, 0);
		}
	}

	if( _advanced )
	{
		_timeBurn += dt;
		while( _timeBurn > 0 )
		{
			GC_FireSpark *tmp = new GC_FireSpark(GetPos() + g_level->net_vrand(33), SPEED_SMOKE/2, veh, true);
			tmp->TimeStepFixed(_timeBurn);
			tmp->SetLifeTime(0.3f);
			_timeBurn -= 0.01f;
		}
	}

	GC_Weapon::TimeStepFixed(dt);
}

///////////////////////////////////////////////////////////////////////////////
// end of file
