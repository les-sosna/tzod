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

GC_Weapon::GC_Weapon(float x, float y) : GC_Pickup(x, y), _rotator(_angle)
{
	SetRespawnTime( GetDefaultRespawnTime() );
	SetAutoSwitch(false);

	_advanced     = false;
	_feTime       = 1.0f;
	_feOrient     = 0;
	_fePos.Set(0,0);

	_time       = 0;
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
	GC_Pickup::Attach(actor);

	_ASSERT(dynamic_cast<GC_Vehicle*>(actor));
	GC_Vehicle *veh = static_cast<GC_Vehicle*>(actor);

	SetZ(Z_ATTACHED_ITEM);

	_rotateSound = new GC_Sound(SND_TowerRotate, SMODE_STOP, GetPos());
	_rotator.reset(0, 0, TOWER_ROT_SPEED, TOWER_ROT_ACCEL, TOWER_ROT_SLOWDOWN);

	Show(true);
	SetBlinking(false);

	SetCrosshair();
	if( _crosshair )
	{
		if( GC_Vehicle *veh = dynamic_cast<GC_Vehicle*>(GetOwner()) )
		{
			_crosshair->Show(NULL != dynamic_cast<GC_PlayerLocal*>(veh->GetPlayer()));
		}
		else
		{
			_crosshair->Show(false);
		}
	}


	PLAY(SND_w_Pickup, GetPos());

	_fireEffect = new GC_UserSprite();
	_fireEffect->SetZ(Z_EXPLODE);
	_fireEffect->Show(false);

	_fireLight = new GC_Light(GC_Light::LIGHT_POINT);
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
	if( IsAttached() )
	{
		const VehicleState &vs = static_cast<GC_Vehicle*>(GetOwner())->_state;
		if( vs._bExplicitTower )
		{
			_rotator.rotate_to( vs._fTowerAngle );
		}
		else
		{
			if( vs._bState_TowerCenter )
				_rotator.rotate_to( 0.0f );
			else if( vs._bState_TowerLeft )
				_rotator.rotate_left();
			else if( vs._bState_TowerRight )
				_rotator.rotate_right();
			else if( RS_GETTING_ANGLE != _rotator.GetState() )
				_rotator.stop();
		}
		_rotator.setup_sound(GetRawPtr(_rotateSound));
	}
}

void GC_Weapon::SetCrosshair()
{
	_crosshair = new GC_Crosshair(GC_Crosshair::CHS_SINGLE);
}

GC_Weapon::GC_Weapon(FromFile) : GC_Pickup(FromFile()), _rotator(_angle)
{
}

GC_Weapon::~GC_Weapon()
{
}

void GC_Weapon::Serialize(SaveFile &f)
{
	GC_Pickup::Serialize(f);

	_rotator.Serialize(f);

	f.Serialize(_angle);
	f.Serialize(_advanced);
	f.Serialize(_feOrient);
	f.Serialize(_fePos);
	f.Serialize(_feTime);
	f.Serialize(_time);
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
	_ASSERT(!_crosshair);
	_ASSERT(!_rotateSound);
	_ASSERT(!_fireEffect);

	GC_Pickup::Kill();
}

void GC_Weapon::UpdateView()
{
	float a = static_cast<GC_Vehicle*>(GetOwner())->GetRotation() + _angle;

	SetRotation(a);
	if( _fireEffect->IsVisible() )
	{
		int frame = int( _time / _feTime * (float) _fireEffect->GetFrameCount() );
		if( frame < _fireEffect->GetFrameCount() )
		{
			float op = 1.0f - powf(_time / _feTime, 2);

			_fireEffect->SetFrame(frame);
			_fireEffect->SetRotation(a + _feOrient);
			_fireEffect->SetOpacity(op);

			float s = sinf(a);
			float c = cosf(a);
			_fireEffect->MoveTo(GetPos() +
				vec2d(_fePos.x*c + _fePos.y*s, _fePos.x*s - _fePos.y*c));

			_fireLight->MoveTo(_fireEffect->GetPos());
			_fireLight->SetIntensity(op);
			_fireLight->Activate(true);
		}
		else
		{
			_fireEffect->SetFrame(0);
			_fireEffect->Show(false);
			_fireLight->Activate(false);
		}
	}
}

void GC_Weapon::TimeStepFixed(float dt)
{
	GC_Pickup::TimeStepFixed(dt);

	_time += dt;

	if( IsAttached() )
	{
		_rotator.process_dt(dt);
		ProcessRotate(dt);
		UpdateView();
		if( _crosshair && GC_Crosshair::CHS_SINGLE == _crosshair->_chStyle )
		{
			_crosshair->MoveTo(GetPos() + CH_DISTANCE_NORMAL * vec2d(
				static_cast<GC_Vehicle*>(GetOwner())->_angle + _angle) );
		}
	}
	else
	{
		if( GetRespawn() && IsVisible() )
		{
			SetBlinking(_time > 12.0f);
			if( _time > 15.0f )
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
		SetRotation(GetTimeAnimation());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_RocketLauncher)
{
	ED_ITEM("weap_rockets", "Оружие:\tРакетница", 4 );
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

GC_Weap_RocketLauncher::GC_Weap_RocketLauncher(float x, float y) : GC_Weapon(x, y)
{
	_firing = false;
	_feTime  = 0.1f;
	SetTexture("weap_ak47");
}

GC_Weap_RocketLauncher::GC_Weap_RocketLauncher(FromFile) : GC_Weapon(FromFile())
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
	_ASSERT(IsAttached());
	float a = static_cast<GC_Vehicle*>(GetOwner())->GetRotation() + _angle;

	if( _advanced )
	{
		if( _time >= _time_shot )
		{
			float dang = g_level->net_frand(0.1f) - 0.05f;
			float dy = (((float)(g_level->net_rand()%(_nshots_total+1)) - 0.5f) /
				(float)_nshots_total - 0.5f) * 18.0f;
			_fePos.Set(13, dy);

			float ax = cosf(a) * 15.0f + dy * sinf(a);
			float ay = sinf(a) * 15.0f - dy * cosf(a);

			new GC_Rocket(GetOwner()->GetPos() + vec2d(ax, ay),
						  vec2d(a + dang) * SPEED_ROCKET,
						  static_cast<GC_Vehicle*>(GetOwner()), _advanced );

			_time    = 0;
			_nshots  = 0;
			_firing = false;

			_fireEffect->Show(true);
		}
	}
	else
	{
		if( _firing )
		{
			if( _time >= _time_shot )
			{
				_nshots++;

				float dang = g_level->net_frand(0.1f) - 0.05f;
				float dy = (((float)_nshots - 0.5f) / (float)_nshots_total - 0.5f) * 18.0f;
				_fePos.Set(13, dy);

				if( _nshots == _nshots_total )
				{
					_firing = false;
					_nshots = 0;
				}

				float ax = cosf(a) * 15.0f + dy * sinf(a);
				float ay = sinf(a) * 15.0f - dy * cosf(a);

				new GC_Rocket( GetOwner()->GetPos() + vec2d(ax, ay),
				               vec2d(a + dang) * SPEED_ROCKET,
				               static_cast<GC_Vehicle*>(GetOwner()), _advanced );

				_time = 0;
				_fireEffect->Show(true);
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
	pSettings->fMaxAttackAngle    = 0.2f;
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
	ED_ITEM( "weap_autocannon", "Оружие:\tАвтоматическая пушка", 4 );
	return true;
}

GC_Weap_AutoCannon::GC_Weap_AutoCannon(float x, float y) : GC_Weapon(x, y)
{
	_feTime = 0.2f;
	SetTexture("weap_ac");
}

void GC_Weap_AutoCannon::SetAdvanced(bool advanced)
{
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(this, LOCATION_BOTTOM);
	if( pIndicator ) pIndicator->Show(!advanced);
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
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(this, LOCATION_BOTTOM);
	if (pIndicator) pIndicator->Kill();

	// убиваем звук перезарядки
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

GC_Weap_AutoCannon::GC_Weap_AutoCannon(FromFile) : GC_Weapon(FromFile())
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
	float a = static_cast<GC_Vehicle*>(GetOwner())->GetRotation() + _angle;

	if( _firing && IsAttached() )
	{
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
				_fireEffect->Show(true);
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
				_fireEffect->Show(true);

				PulseNotify(NOTIFY_OBJECT_UPDATE_INDICATOR);
			}
		}
	}
}

void GC_Weap_AutoCannon::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngle    = 0.1f;
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
			PulseNotify(NOTIFY_OBJECT_UPDATE_INDICATOR);
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
	ED_ITEM( "weap_cannon", "Оружие:\tТяжелая пушка", 4 );
	return true;
}

GC_Weap_Cannon::GC_Weap_Cannon(float x, float y) : GC_Weapon(x, y)
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

GC_Weap_Cannon::GC_Weap_Cannon(FromFile) : GC_Weapon(FromFile())
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
		vec2d dir(veh->GetRotation() + _angle);

		new GC_TankBullet(GetPos() + dir * 17.0f, dir * SPEED_TANKBULLET + g_level->net_vrand(50),
			veh, _advanced );

		if( !_advanced )
		{
			veh->ApplyImpulse( dir * (-80.0f) );
		}

		_time       = 0;
		_time_smoke = 0.3f;

		_fireEffect->Show(true);
	}
}

void GC_Weap_Cannon::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngle    = 0.1f;
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

	if( !IsAttached() ) return;

	if( _time_smoke > 0 )
	{
		_time_smoke -= dt;
		_time_smoke_dt += dt;

		for( ;_time_smoke_dt > 0; _time_smoke_dt -= 0.025f )
		{
			vec2d a(static_cast<GC_Vehicle*>(GetOwner())->GetRotation() + _angle);
			new GC_Particle(GetPos() + a * 26.0f, SPEED_SMOKE + a * 50.0f, tex, frand(0.3f) + 0.2f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Plazma)
{
	ED_ITEM( "weap_plazma", "Оружие:\tПлазменная пушка", 4 );
	return true;
}

GC_Weap_Plazma::GC_Weap_Plazma(float x, float y) : GC_Weapon(x, y)
{
	SetTexture("weap_plazma");
	_fePos.Set(0, 0);
	_feTime = 0.2f;
}

GC_Weap_Plazma::GC_Weap_Plazma(FromFile) : GC_Weapon(FromFile())
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
		vec2d a(static_cast<GC_Vehicle*>(GetOwner())->GetRotation() + _angle);
		new GC_PlazmaClod(GetPos() + a * 15.0f,
			a * SPEED_PLAZMA + g_level->net_vrand(20),
			static_cast<GC_Vehicle*>(GetOwner()),
			_advanced );
		_time = 0;
		_fireEffect->Show(true);
	}
}

void GC_Weap_Plazma::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngle    = 0.2f;
	pSettings->fProjectileSpeed   = SPEED_PLAZMA;
	pSettings->fAttackRadius_max  = 300;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit = 0;
	pSettings->fDistanceMultipler = _advanced ? 2.0f : 8.0f;  // fixme
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Gauss)
{
	ED_ITEM( "weap_gauss", "Оружие:\tПушка Гаусса", 4 );
	return true;
}

GC_Weap_Gauss::GC_Weap_Gauss(float x, float y) : GC_Weapon(x, y)
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

GC_Weap_Gauss::GC_Weap_Gauss(FromFile) : GC_Weapon(FromFile())
{
}

GC_Weap_Gauss::~GC_Weap_Gauss()
{
}

void GC_Weap_Gauss::Fire()
{
	if( IsAttached() && _time >= _timeReload )
	{
		float a = static_cast<GC_Vehicle*>(GetOwner())->GetRotation() + _angle;
		float s = sinf(a);
		float c = cosf(a);

		new GC_GaussRay(vec2d(GetPos().x + c + 5 * s, GetPos().y + s - 5 * c),
			vec2d(c, s) * SPEED_GAUSS,
			static_cast<GC_Vehicle*>(GetOwner()), _advanced );

		_time = 0;
		_fireEffect->Show(true);
	}
}

void GC_Weap_Gauss::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = FALSE;
	pSettings->fMaxAttackAngle    = 0.01f;
	pSettings->fProjectileSpeed   = 0;
	pSettings->fAttackRadius_max  = 800;
	pSettings->fAttackRadius_min  = 400;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = _advanced ? 4.5f : 9.0f;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Ram)
{
	ED_ITEM( "weap_ram", "Оружие:\tТаран", 4 );
	return true;
}

GC_Weap_Ram::GC_Weap_Ram(float x, float y) : GC_Weapon(x, y)
{
	_engineSound = NULL;
	_bFire        = false;
	SetTexture("weap_ram");
}

void GC_Weap_Ram::SetAdvanced(bool advanced)
{
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(this, LOCATION_BOTTOM);
	if( pIndicator ) pIndicator->Show(!advanced);

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

	_engineSound = new GC_Sound(SND_RamEngine, SMODE_STOP, GetPos());

	_engineLight = new GC_Light(GC_Light::LIGHT_POINT);
	_engineLight->SetIntensity(1.0f);
	_engineLight->SetRadius(120);
	_engineLight->Activate(false);


	_fuel_max  = _fuel = 1.0f;
	_fuel_rate = 0.2f;
	_fuel_rep  = 0.1f;

	_bFire  = false;
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

GC_Weap_Ram::GC_Weap_Ram(FromFile) : GC_Weapon(FromFile())
{
}

GC_Weap_Ram::~GC_Weap_Ram()
{
}

void GC_Weap_Ram::UpdateView()
{
	GC_Weapon::UpdateView();
	_engineLight->MoveTo(GetPos()-vec2d(GetRotation())*20);
}

void GC_Weap_Ram::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_bFire);
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
	_ASSERT(IsAttached());

	if( _bReady )
	{
		_bFire = true;
		if( GC_RigidBodyDynamic *owner = dynamic_cast<GC_RigidBodyDynamic *>(GetOwner()) )
		{
			owner->ApplyForce( vec2d(_angle + owner->_angle) * 1000 );
		}
	}
}

void GC_Weap_Ram::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = FALSE;
	pSettings->fMaxAttackAngle    = 0.3f;
	pSettings->fProjectileSpeed   = 0;
	pSettings->fAttackRadius_max  = 100;
	pSettings->fAttackRadius_min  = 0;
	pSettings->fAttackRadius_crit = 0;
	pSettings->fDistanceMultipler = _advanced ? 2.5f : 6.0f;
}

void GC_Weap_Ram::TimeStepFixed(float dt)
{
	static const TextureCache tex1("particle_fire2");
	static const TextureCache tex2("particle_yellow");
	static const TextureCache tex3("particle_fire");

	GC_Weapon::TimeStepFixed( dt );

	if( IsAttached() )
	{
		_ASSERT(_engineSound);

		if( _advanced )
			_fuel = _fuel_max;

		if( _bFire )
		{
			_engineSound->Pause(false);
			_engineSound->MoveTo(GetPos());

			_fuel = __max(0, _fuel - _fuel_rate * dt);
			if( 0 == _fuel ) _bReady = false;

			GC_Vehicle *veh = static_cast<GC_Vehicle *>(GetOwner());

			vec2d v = veh->_lv;

			// основная струя
			{
				const float lenght = 50.0f;
				vec2d a(veh->_angle + _angle);

				vec2d emitter = GetPos() - a * 20.0f;
				vec2d hit;
				GC_RigidBodyStatic *object = g_level->agTrace (
					g_level->grid_rigid_s, veh, emitter, -a * lenght, &hit);
				if( object )
				{
					object->TakeDamage(
						dt * DAMAGE_RAM_ENGINE * (1.0f - (hit - emitter).len() / lenght),
						hit, veh);
				}

				for( int i = 0; i < 29; ++i )
				{
					float time = frand(0.05f) + 0.02f;
					float t = (frand(1.0f) - 0.5f) * 6.0f;
					vec2d dx( -a.y * t, a.x * t);

					new GC_Particle(emitter + dx, v - a * (frand(800.0f)) - dx / time,
						fabsf(t) > 1.5 ? tex1 : tex2, time);
				}
			}

			// боковые струи
			for( float l = -1; l < 2; l += 2 )
			{
				const float lenght = 50.0f;
				vec2d a(veh->_angle + _angle + l * 0.15f);

				vec2d emitter = GetPos() - a * 15.0f + vec2d( -a.y, a.x) * l * 17.0f;
				vec2d hit;
				GC_RigidBodyStatic *object = g_level->agTrace(g_level->grid_rigid_s,
					veh, emitter + a * 2.0f, -a * lenght, &hit);
				if( object )
				{
					object->TakeDamage(
						dt * DAMAGE_RAM_ENGINE * (1.0f - (hit - emitter).len() / lenght),
						hit, veh);
				}

				for( int i = 0; i < 10; i++ )
				{
					float time = frand(0.05f) + 0.02f;
					float t = (frand(1.0f) - 0.5f) * 2.5f;
					vec2d dx( -a.y * t, a.x * t);

					new GC_Particle(emitter + dx, v - a * (frand(600.0f)) - dx / time, tex3, time);
				}
			}
		}
		else
		{
			_engineSound->Pause(true);
			_fuel   = __min(_fuel_max, _fuel + _fuel_rep * dt);
			_bReady = (_fuel_max < _fuel * 4.0f);
		}

		_engineLight->Activate(_bFire);
		_bFire = false;
	}
	else
	{
		_ASSERT(!_engineSound);
	}

	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_BFG)
{
	ED_ITEM( "weap_bfg", "Оружие:\tБФГ", 4 );
	return true;
}

GC_Weap_BFG::GC_Weap_BFG(float x, float y) : GC_Weapon(x, y)
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

GC_Weap_BFG::GC_Weap_BFG(FromFile) : GC_Weapon(FromFile())
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
	_ASSERT(IsAttached());

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
			vec2d a((veh ? veh->_angle : 0) + _angle);

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
	pSettings->fMaxAttackAngle    = 0.01f;
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
	ED_ITEM( "weap_ripper", "Оружие:\tРипер", 4 );
	return true;
}

void GC_Weap_Ripper::UpdateDisk()
{
	_disk->Show(_time > _timeReload);
	_disk->MoveTo(GetPos() - vec2d(GetRotation()) * 8);
}

void GC_Weap_Ripper::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

	_timeReload = 0.5f;
	_disk = new GC_UserSprite();
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

GC_Weap_Ripper::GC_Weap_Ripper(FromFile) : GC_Weapon(FromFile())
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
		vec2d a((veh ? veh->_angle : 0) + _angle);

		new GC_Disk(GetPos() - a * 9.0f, a * SPEED_DISK + g_level->net_vrand(10),
			dynamic_cast<GC_RigidBodyStatic*>(GetOwner()), _advanced );
		PLAY(SND_DiskFire, GetPos());

		_time = 0;
	}
}

void GC_Weap_Ripper::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngle    = 0.2f;
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
		_disk->SetRotation(_disk->GetRotation() + dt * 10);
		UpdateDisk();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Minigun)
{
	ED_ITEM( "weap_minigun", "Оружие:\tПулемет", 4 );
	return true;
}

GC_Weap_Minigun::GC_Weap_Minigun(float x, float y) : GC_Weapon(x, y)
{
	_bFire = false;

	SetTexture("weap_mg1");

	_fePos.Set(20, 0);
	_feTime   = 0.1f;
	_feOrient = frand(PI2);
}

GC_Weap_Minigun::GC_Weap_Minigun(FromFile) : GC_Weapon(FromFile())
{
}

GC_Weap_Minigun::~GC_Weap_Minigun()
{
}

void GC_Weap_Minigun::Attach(GC_Actor *actor)
{
	GC_Weapon::Attach(actor);

	_timeReload = 0.03f;
	_time_rotate = 0;
	_time_fire   = 0;
	_time_shot   = 0;

	_sound = new GC_Sound(SND_MinigunFire, SMODE_STOP, GetPos());
	_bFire = false;

	_fireEffect->SetTexture("minigun_fire");


	if( _crosshair_left )
	{
		if( GC_Vehicle *veh = dynamic_cast<GC_Vehicle*>(GetOwner()) )
		{
			_crosshair_left->Show(NULL != dynamic_cast<GC_PlayerLocal*>(veh->GetPlayer()));
		}
		else
		{
			_crosshair_left->Show(false);
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
	SAFE_KILL(_crosshair_left);
	SAFE_KILL(_sound);

	GC_Weapon::Detach();
}

void GC_Weap_Minigun::SetCrosshair()
{
	_crosshair      = new GC_Crosshair(GC_Crosshair::CHS_DOUBLE);
	_crosshair_left = new GC_Crosshair(GC_Crosshair::CHS_DOUBLE);
}

void GC_Weap_Minigun::Serialize(SaveFile &f)
{
	GC_Weapon::Serialize(f);

	f.Serialize(_bFire);
	f.Serialize(_time_fire);
	f.Serialize(_time_rotate);
	f.Serialize(_time_shot);
	f.Serialize(_crosshair_left);
	f.Serialize(_sound);
}

void GC_Weap_Minigun::Kill()
{
	SAFE_KILL(_crosshair_left);
	SAFE_KILL(_sound);

	GC_Weapon::Kill();
}

void GC_Weap_Minigun::Fire()
{
	_ASSERT(IsAttached());
	if( IsAttached() )
		_bFire = true;
}

void GC_Weap_Minigun::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = TRUE;
	pSettings->fMaxAttackAngle    = 0.3f;
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
		float va = veh ? veh->_angle : 0;

		if( _bFire )
		{
			_time_rotate += dt;
			_time_shot   += dt;

			SetTexture((fmodf(_time_rotate, 0.08f) < 0.04f) ? "weap_mg1":"weap_mg2");

			_sound->MoveTo(GetPos());
			_sound->Pause(false);
			_bFire = false;

			for(; _time_shot > 0; _time_shot -= _advanced ? 0.02f : 0.04f)
			{
				_time = frand(_feTime);
				_feOrient = frand(PI2);
				_fireEffect->Show(true);

				float da = _time_fire * 0.07f / WEAP_MG_TIME_RELAX;

				vec2d a(va + _angle + g_level->net_frand(da * 2.0f) - da);
				a *= (1 - g_level->net_frand(0.2f));

				new GC_Bullet(GetPos() + a * 18.0f, a * SPEED_BULLET, veh, _advanced );

				if( veh && !_advanced )
				{
					if( g_level->net_frand(WEAP_MG_TIME_RELAX * 5.0f) < _time_fire - WEAP_MG_TIME_RELAX * 0.2f )
					{
						float m = 3000;//veh->_inv_i;
						veh->ApplyTorque(m * (g_level->net_frand(1.0f) - 0.5f));
					}
				}
			}

			_time_fire = __min(_time_fire + dt * 2, WEAP_MG_TIME_RELAX);
		}
		else
		{
			_sound->Pause(true);
			_time_fire = __max(_time_fire - dt, 0);
		}

		float da = _time_fire * 0.1f / WEAP_MG_TIME_RELAX;
		if( _crosshair )
		{
			_crosshair->_angle = va + da + _angle;
			_crosshair->MoveTo(GetPos() + vec2d(_crosshair->_angle) * CH_DISTANCE_THIN);
		}

		if( _crosshair_left )
		{
			_crosshair_left->_angle = va - da + _angle;
			_crosshair_left->MoveTo(GetPos() + vec2d(_crosshair_left->_angle) * CH_DISTANCE_THIN);
		}
	}

	GC_Weapon::TimeStepFixed(dt);
}

///////////////////////////////////////////////////////////////////////////////
// end of file
