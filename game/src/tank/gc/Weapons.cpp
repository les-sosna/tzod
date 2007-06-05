// Weapons.cpp

#include "stdafx.h"

#include "Weapons.h"
#include "Vehicle.h"
#include "Sound.h"
#include "Light.h"
#include "Player.h"
#include "GameClasses.h"
#include "Indicators.h"
#include "Projectiles.h"
#include "Particles.h"
#include "ai.h"

#include "Macros.h"
#include "Level.h"
#include "functions.h"

#include "fs/SaveFile.h"

#include "Options.h"

///////////////////////////////////////////////////////////////////////////////

GC_Weapon::GC_Weapon(float x, float y) : GC_PickUp(x, y), _rotator(_angle)
{
	_advanced      = false;
	_feTime        = 1.0f;
	_time_respawn  = GetDefaultRespawnTime();
	_feOrient      = 0;
	_fePos.Set(0,0);

	_time = 0;
	_bMostBeAllowed = true;
}

AIPRIORITY GC_Weapon::CheckUseful(GC_Vehicle *pVehicle)
{
	if( pVehicle->_weapon )
	{
		if( pVehicle->_weapon->_advanced )
			return AIP_NOTREQUIRED;

		if( _advanced )
			return AIP_WEAPON_ADVANCED;
		else
			return AIP_NOTREQUIRED;
	}

	return AIP_WEAPON_NORMAL + _advanced ? AIP_WEAPON_ADVANCED : AIP_NOTREQUIRED;
}

void GC_Weapon::GiveIt(GC_Vehicle* pVehicle)
{
	Attach(pVehicle);
	GC_PickUp::GiveIt(pVehicle);
}

void GC_Weapon::Attach(GC_Vehicle *pVehicle)
{
	SetZ(Z_ATTACHED_ITEM);

	_rotateSound = new GC_Sound(SND_TowerRotate, SMODE_STOP, _pos);
	_rotator.reset(0, 0, TOWER_ROT_SPEED, TOWER_ROT_ACCEL, TOWER_ROT_SLOWDOWN);

	GC_Weapon *pOldWeap = GetRawPtr(pVehicle->_weapon);
	if( pOldWeap )
	{
		pVehicle->_weapon->Detach();
		pOldWeap->Kill();
	}

	_proprietor = pVehicle;
	_proprietor->_weapon = this;
	_proprietor->Subscribe(NOTIFY_OBJECT_MOVE, this,
		(NOTIFYPROC) &GC_Weapon::OnProprietorMove, false, false);


	_bAttached = true;

	Show(true);
	SetBlinking(false);

	SetCrosshair();

	PLAY(SND_w_PickUp, _pos);

	_fireEffect = new GC_UserSprite();
	_fireEffect->SetZ(Z_EXPLODE);
	_fireEffect->Show(false);

	_fireLight = new GC_Light(GC_Light::LIGHT_POINT);
	_fireLight->Enable(false);

	//

	VehicleClass vc;

	lua_State *L = LS(g_env.hScript);
	lua_pushcfunction(L, luaT_ConvertVehicleClass); // function to call
	lua_getglobal(L, "getvclass");
	lua_pushstring(L, _proprietor->_player->_class.c_str());  // cls arg
	lua_pushstring(L, g_level->GetTypeName(GetType()));       // weap arg
	if( lua_pcall(L, 2, 1, 0) )
	{
		// print error message
		_MessageArea::Inst()->message(lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	lua_pushlightuserdata(L, &vc);
	if( lua_pcall(L, 2, 0, 0) )
	{
		// print error message
		_MessageArea::Inst()->message(lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	_proprietor->SetClass(vc);
}

void GC_Weapon::Detach()
{
	SetZ(Z_FREE_ITEM);

	if( _proprietor )
	{
		_proprietor->Unsubscribe(this);
		_proprietor->_weapon = NULL;
		_proprietor->_player->ResetClass();
		_proprietor = NULL;
	}

	SAFE_KILL(_rotateSound);
	SAFE_KILL(_crosshair);
	SAFE_KILL(_fireEffect);
	SAFE_KILL(_fireLight);

	_bAttached = false;
	_time = 0;
}

void GC_Weapon::ProcessRotate(float dt)
{
	if( _bAttached )
	{
		_ASSERT(_proprietor);

		if( _proprietor->_state._bExplicitTower )
		{
			_rotator.rotate_to( _proprietor->_state._fTowerAngle );
		}
		else
		{
			if( _proprietor->_state._bState_TowerCenter )
				_rotator.rotate_to( 0.0f );
			else if( _proprietor->_state._bState_TowerLeft )
				_rotator.rotate_left();
			else if( _proprietor->_state._bState_TowerRight )
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

GC_Weapon::GC_Weapon(FromFile) : GC_PickUp(FromFile()), _rotator(_angle)
{
}

GC_Weapon::~GC_Weapon()
{
}

void GC_Weapon::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_PickUp::Serialize(f);
	/////////////////////////////////////
	_rotator.Serialize(f);
	/////////////////////////////////////
	f.Serialize(_angle);
	f.Serialize(_advanced);
	f.Serialize(_feOrient);
	f.Serialize(_fePos);
	f.Serialize(_feTime);
	f.Serialize(_time);
	f.Serialize(_time_reload);
	/////////////////////////////////////
	f.Serialize(_crosshair);
	f.Serialize(_fireEffect);
	f.Serialize(_fireLight);
	f.Serialize(_proprietor);
	f.Serialize(_rotateSound);
}

void GC_Weapon::Kill()
{
	if( _bAttached ) Detach();

	_ASSERT(!_crosshair);
	_ASSERT(!_rotateSound);
	_ASSERT(!_fireEffect);

	GC_PickUp::Kill();
}

void GC_Weapon::UpdateView()
{
	SetRotation(_proprietor->_angle + _angle);
	if( _fireEffect->IsVisible() )
	{
		int frame = int( _time / _feTime * (float) _fireEffect->GetFrameCount() );
		if( frame < _fireEffect->GetFrameCount() )
		{
			float op = 1.0f - powf(_time / _feTime, 2);

			_fireEffect->SetFrame(frame);
			_fireEffect->SetRotation(_proprietor->_angle + _angle + _feOrient);
			_fireEffect->SetOpacity(op);
			float s = sinf(_proprietor->_angle + _angle);
			float c = cosf(_proprietor->_angle + _angle);
			_fireEffect->MoveTo(_pos +
				vec2d(_fePos.x*c + _fePos.y*s, _fePos.x*s - _fePos.y*c));

			_fireLight->MoveTo(_fireEffect->_pos);
			_fireLight->SetIntensity(op);
			_fireLight->Enable(true);
		}
		else
		{
			_fireEffect->SetFrame(0);
			_fireEffect->Show(false);
			_fireLight->Enable(false);
		}
	}
}

void GC_Weapon::TimeStepFixed(float dt)
{
	GC_PickUp::TimeStepFixed(dt);

	_time += dt;

	if( !_bAttached && !_bRespawn )
	{
		SetBlinking(_time > 12.0f);
		if( _time > 15.0f ) Kill();
	}
	else
	{
		if( _bAttached )
		{
			if( _crosshair )
			{
				_crosshair->Show(!OPT(players)[_proprietor->_player->_nIndex].bAI);
			}

			_rotator.process_dt(dt);
			ProcessRotate(dt);

			if( _proprietor->_state._bState_Fire && !g_level->_limitHit )
			{
				Fire();
			}

			UpdateView();
		}
	}
}

void GC_Weapon::TimeStepFloat(float dt)
{
	GC_PickUp::TimeStepFloat(dt);

    if( !_bAttached && _bRespawn )
		SetRotation(_time_animation);
}

void GC_Weapon::OnProprietorMove(GC_Vehicle *sender, void *param)
{
	_ASSERT(_bAttached);

	MoveTo(_proprietor->_pos);
	UpdateView();

	if( _crosshair && GC_Crosshair::CHS_SINGLE == _crosshair->_chStyle )
	{
		_crosshair->MoveTo(_pos + vec2d(_proprietor->_angle + _angle) * CH_DISTANCE_NORMAL );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_RocketLauncher)
{
	ED_ITEM("weap_rockets", "Оружие:\tРакетница" );
	return true;
}

void GC_Weap_RocketLauncher::Attach(GC_Vehicle *pVehicle)
{
	GC_Weapon::Attach(pVehicle);

	_time = _time_reload = 2.0f;

	_reloaded          = true;
	_firing           = false;
	_nshots            = 0;
	_nshots_total      = 6;
	_time_shot         = 0.13f;

	_fireEffect->SetTexture("particle_fire3");

return;
	pVehicle->SetMaxHP(85);

	pVehicle->_ForvAccel = 300;
	pVehicle->_BackAccel = 200;
	pVehicle->_StopAccel = 500;

//	pVehicle->_rotator.setl(3.5f, 10.0f, 30.0f);

	pVehicle->_MaxBackSpeed = 150;
	pVehicle->_MaxForvSpeed = 150;
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

GC_PickUp* GC_Weap_RocketLauncher::SetRespawn()
{
	return new GC_Weap_RocketLauncher(_pos.x, _pos.y);
}

void GC_Weap_RocketLauncher::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_Weapon::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_firing);
	f.Serialize(_reloaded);
	f.Serialize(_nshots);
	f.Serialize(_nshots_total);
	f.Serialize(_time_shot);
}

void GC_Weap_RocketLauncher::Fire()
{
	_ASSERT(_bAttached);

	if( _advanced )
	{
		if( _time >= _time_shot )
		{
			float dang = net_frand(0.1f) - 0.05f;
			float dy = (((float)(net_rand()%(_nshots_total+1)) - 0.5f) /
				(float)_nshots_total - 0.5f) * 18.0f;
			_fePos.Set(13, dy);

			float ax = cosf(_proprietor->_angle + _angle) * 15.0f + dy * sinf(_proprietor->_angle + _angle);
			float ay = sinf(_proprietor->_angle + _angle) * 15.0f - dy * cosf(_proprietor->_angle + _angle);

			new GC_Rocket(_proprietor->_pos + vec2d(ax, ay),
						  vec2d(_proprietor->_angle + dang + _angle) * SPEED_ROCKET,
						  GetRawPtr(_proprietor), _advanced );

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

				float dang = net_frand(0.1f) - 0.05f;
				float dy = (((float)_nshots - 0.5f) / (float)_nshots_total - 0.5f) * 18.0f;
				_fePos.Set(13, dy);

				if( _nshots == _nshots_total )
				{
					_firing = false;
					_nshots = 0;
				}

				float ax = cosf(_proprietor->_angle + _angle) * 15.0f + dy * sinf(_proprietor->_angle + _angle);
				float ay = sinf(_proprietor->_angle + _angle) * 15.0f - dy * cosf(_proprietor->_angle + _angle);

				new GC_Rocket(_proprietor->_pos + vec2d(ax, ay),
							  vec2d(_proprietor->_angle + dang + _angle) * SPEED_ROCKET,
							  GetRawPtr(_proprietor), _advanced );

				_time = 0;
				_fireEffect->Show(true);
			}

		}

		if( _time >= _time_reload )
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
	if( _bAttached )
	{
		if( _firing )
			Fire();
		else if( _time >= _time_reload && !_reloaded )
		{
			_reloaded = true;
			if( !_advanced) PLAY(SND_WeapReload, _pos);
		}
	}

	GC_Weapon::TimeStepFixed(dt);
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_AutoCannon)
{
	ED_ITEM( "weap_autocannon", "Оружие:\tАвтоматическая пушка" );
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

void GC_Weap_AutoCannon::Attach(GC_Vehicle *pVehicle)
{
	GC_Weapon::Attach(pVehicle);

	_time_reload = 3.7f;
	_time = _time_reload;

	_firing = false;
	_nshots = 0;
	_nshots_total = 30;
	_time_shot = 0.135f;

	GC_IndicatorBar *pIndicator = new GC_IndicatorBar("indicator_ammo", this,
		(float *) &_nshots, (float *) &_nshots_total, LOCATION_BOTTOM);
	pIndicator->SetInverse(true);

	_fireEffect->SetTexture("particle_fire3");

return;
	pVehicle->SetMaxHP(80);

	pVehicle->_ForvAccel = 300;
	pVehicle->_BackAccel = 200;
	pVehicle->_StopAccel = 500;

//	pVehicle->_rotator.setl(3.5f, 10.0f, 30.0f);

	pVehicle->_MaxForvSpeed = 240;
	pVehicle->_MaxBackSpeed = 160;
}

void GC_Weap_AutoCannon::Detach()
{
	GC_IndicatorBar *pIndicator = GC_IndicatorBar::FindIndicator(this, LOCATION_BOTTOM);
	if (pIndicator) pIndicator->Kill();

	// убиваем звук перезарядки
	FOREACH( sounds, GC_Sound, object )
	{
		if( GC_Sound_link::this_type == object->GetType() )
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

GC_PickUp* GC_Weap_AutoCannon::SetRespawn()
{
	return new GC_Weap_AutoCannon(_pos.x, _pos.y);
}

void GC_Weap_AutoCannon::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_Weapon::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_firing);
	f.Serialize(_nshots);
	f.Serialize(_nshots_total);
	f.Serialize(_time_shot);
}

void GC_Weap_AutoCannon::Fire()
{
	if( _firing && _bAttached )
	{
		if( _advanced )
		{
			if( _time >= _time_shot )
			{
				for( int t = 0; t < 2; ++t )
				{
					float dy = t == 0 ? -9.0f : 9.0f;

					float ax = cosf(_proprietor->_angle + _angle) * 17.0f - dy * sinf(_proprietor->_angle + _angle);
					float ay = sinf(_proprietor->_angle + _angle) * 17.0f + dy * cosf(_proprietor->_angle + _angle);

					new GC_ACBullet(_proprietor->_pos + vec2d(ax, ay),
									vec2d(_proprietor->_angle + _angle) * SPEED_ACBULLET,
									GetRawPtr(_proprietor), _advanced );
				}

				_time   = 0;
				PLAY(SND_ACShoot, _pos);

				_fePos.Set(17.0f, 0);
				_fireEffect->Show(true);
			}
		}
		else
		{
			if( _time >= _time_shot )
			{
				_nshots++;

				float dang = net_frand(0.02f) - 0.01f;
				float dy = _nshots%2 == 0 ? -9.0f : 9.0f;

				if( _nshots == _nshots_total )
				{
					_firing = false;
					new GC_Sound_link(SND_AC_Reload, SMODE_PLAY, this);
				}


				float ax = cosf(_proprietor->_angle + _angle) * 17.0f - dy * sinf(_proprietor->_angle + _angle);
				float ay = sinf(_proprietor->_angle + _angle) * 17.0f + dy * cosf(_proprietor->_angle + _angle);

				new GC_ACBullet(_proprietor->_pos + vec2d(ax, ay),
								vec2d(_proprietor->_angle + dang + _angle) * SPEED_ACBULLET,
								GetRawPtr(_proprietor), _advanced );

				_time = 0;
				PLAY(SND_ACShoot, _pos);

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
	if( _bAttached )
	{
		if( _advanced )
			_nshots  = 0;

		if( _time >= _time_reload && !_firing )
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
	ED_ITEM( "weap_cannon", "Оружие:\tТяжелая пушка" );
	return true;
}

GC_Weap_Cannon::GC_Weap_Cannon(float x, float y) : GC_Weapon(x, y)
{
	_fePos.Set(21, 0);
	_feTime = 0.2f;
	SetTexture("weap_cannon");
}

void GC_Weap_Cannon::Attach(GC_Vehicle *pVehicle)
{
	GC_Weapon::Attach(pVehicle);

	_time_reload   = 0.9f;
	_time_smoke_dt = 0;
	_time_smoke    = 0;

	_fireEffect->SetTexture("particle_fire3");

return;
	pVehicle->SetMaxHP(125);

	pVehicle->_ForvAccel = 250;
	pVehicle->_BackAccel = 150;
	pVehicle->_StopAccel = 500;

//	pVehicle->_rotator.setl(3.5f, 10.0f, 30.0f);

	pVehicle->_MaxForvSpeed = 160;
	pVehicle->_MaxBackSpeed = 120;
}

GC_Weap_Cannon::GC_Weap_Cannon(FromFile) : GC_Weapon(FromFile())
{
}

GC_Weap_Cannon::~GC_Weap_Cannon()
{
}

GC_PickUp* GC_Weap_Cannon::SetRespawn()
{
	return new GC_Weap_Cannon(_pos.x, _pos.y);
}

void GC_Weap_Cannon::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_Weapon::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_time_smoke);
	f.Serialize(_time_smoke_dt);
}

void GC_Weap_Cannon::Fire()
{
	if( _bAttached && _time >= _time_reload )
	{
		vec2d a(_proprietor->_angle + _angle);

		new GC_TankBullet(_pos + a * 17.0f, a * SPEED_TANKBULLET + net_vrand(50),
			GetRawPtr(_proprietor), _advanced );

		if( !_advanced )
		{
			_proprietor->ApplyImpulse( -vec2d(_angle + _proprietor->_angle) * 80 );
		}

		_time = 0;
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

	if( !_bAttached ) return;

	if( _time_smoke > 0 )
	{
		_time_smoke -= dt;
		_time_smoke_dt += dt;

		for( ;_time_smoke_dt > 0; _time_smoke_dt -= 0.025f )
		{
			vec2d a(_proprietor->_angle + _angle);
			new GC_Particle(_pos + a * 26.0f, SPEED_SMOKE + a * 50.0f, tex, frand(0.3f) + 0.2f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Plazma)
{
	ED_ITEM( "weap_plazma", "Оружие:\tПлазменная пушка" );
	return true;
}

GC_Weap_Plazma::GC_Weap_Plazma(float x, float y) : GC_Weapon(x, y)
{
	SetTexture("weap_plazma");
	_fePos.Set(0, 0);
	_feTime = 0.2f;
}

void GC_Weap_Plazma::Attach(GC_Vehicle *pVehicle)
{
	GC_Weapon::Attach(pVehicle);

	_time_reload = 0.3f;
	_fireEffect->SetTexture("particle_plazma_fire");

return;
	pVehicle->SetMaxHP(100);

	pVehicle->_ForvAccel = 300;
	pVehicle->_BackAccel = 200;
	pVehicle->_StopAccel = 500;

//	pVehicle->_rotator.setl(3.5f, 10.0f, 30.0f);

	pVehicle->_MaxForvSpeed = 200;
	pVehicle->_MaxBackSpeed = 160;
}

GC_PickUp* GC_Weap_Plazma::SetRespawn()
{
	return new GC_Weap_Plazma(_pos.x, _pos.y);
}

void GC_Weap_Plazma::Fire()
{
	if( _bAttached && _time >= _time_reload )
	{
		vec2d a(_proprietor->_angle + _angle);

		new GC_PlazmaClod(_pos + a * 15.0f,
			a * SPEED_PLAZMA + net_vrand(20), GetRawPtr(_proprietor), _advanced );

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
	pSettings->fDistanceMultipler = _advanced ? 2.0f : 8.0f;	// fixme
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Gauss)
{
	ED_ITEM( "weap_gauss", "Оружие:\tПушка Гаусса" );
	return true;
}

GC_Weap_Gauss::GC_Weap_Gauss(float x, float y) : GC_Weapon(x, y)
{
	SetTexture("weap_gauss");
	_feTime = 0.15f;
}

void GC_Weap_Gauss::Attach(GC_Vehicle *pVehicle)
{
	GC_Weapon::Attach(pVehicle);

	_time_reload = 1.3f;
	_fireEffect->SetTexture("particle_gaussfire");

return;
	pVehicle->SetMaxHP(70);

	pVehicle->_ForvAccel = 350;
	pVehicle->_BackAccel = 250;
	pVehicle->_StopAccel = 700;

//	pVehicle->_rotator.setl(3.5f, 15.0f, 30.0f);

	pVehicle->_MaxBackSpeed = 220;
	pVehicle->_MaxForvSpeed = 260;
}

GC_Weap_Gauss::GC_Weap_Gauss(FromFile) : GC_Weapon(FromFile())
{
}

GC_Weap_Gauss::~GC_Weap_Gauss()
{
}

GC_PickUp* GC_Weap_Gauss::SetRespawn()
{
	return new GC_Weap_Gauss(_pos.x, _pos.y);
}

void GC_Weap_Gauss::Fire()
{
	if( _bAttached && _time >= _time_reload )
	{
		float s = sinf(_proprietor->_angle + _angle);
		float c = cosf(_proprietor->_angle + _angle);

		new GC_GaussRay(vec2d(_pos.x + c + 5 * s, _pos.y + s - 5 * c),
			vec2d(c, s) * SPEED_GAUSS, GetRawPtr(_proprietor), _advanced );

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

void GC_Weap_Gauss::TimeStepFixed(float dt)
{
	GC_Weapon::TimeStepFixed( dt );

	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Ram)
{
	ED_ITEM( "weap_ram", "Оружие:\tТаран" );
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

	if( _proprietor )
	{
		_proprietor->_percussion =
			advanced ? WEAP_RAM_PERCUSSION * 2 : WEAP_RAM_PERCUSSION;
	}

	GC_Weapon::SetAdvanced(advanced);
}

void GC_Weap_Ram::Attach(GC_Vehicle *pVehicle)
{
	GC_Weapon::Attach(pVehicle);

	_engineSound = new GC_Sound(SND_RamEngine, SMODE_STOP, _pos);

	_engineLight = new GC_Light(GC_Light::LIGHT_POINT);
	_engineLight->SetIntensity(1.0f);
	_engineLight->SetRadius(120);
	_engineLight->Enable(false);


	_fuel_max  = _fuel = 1.0f;
	_fuel_rate = 0.2f;
	_fuel_rep  = 0.1f;

	_bFire  = false;
	_bReady = true;

	GC_IndicatorBar *pIndicator = new GC_IndicatorBar("indicator_fuel", this,
		(float *) &_fuel, (float *) &_fuel_max, LOCATION_BOTTOM);

return;
	pVehicle->SetMaxHP(350);

	pVehicle->_ForvAccel = 250;
	pVehicle->_BackAccel = 250;
	pVehicle->_StopAccel = 500;

	pVehicle->_percussion = _advanced ? WEAP_RAM_PERCUSSION * 2 : WEAP_RAM_PERCUSSION;

//	pVehicle->_rotator.setl(3.5f, 10.0f, 30.0f);

	pVehicle->_MaxBackSpeed = 160;
	pVehicle->_MaxForvSpeed = 160;
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
	_engineLight->MoveTo(_pos-vec2d(GetRotation())*20);
}

GC_PickUp* GC_Weap_Ram::SetRespawn()
{
	return new GC_Weap_Ram(_pos.x, _pos.y);
}

void GC_Weap_Ram::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_Weapon::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_bFire);
	f.Serialize(_bReady);
	f.Serialize(_fuel);
	f.Serialize(_fuel_max);
	f.Serialize(_fuel_rate);
	f.Serialize(_fuel_rep);
	/////////////////////////////////////
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
	_ASSERT(_bAttached);

	if( _bReady )
	{
		_bFire = true;
		_proprietor->ApplyForce( vec2d(_angle + _proprietor->_angle) * 1000 );
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

	if( _bAttached )
	{
		_ASSERT(_engineSound);

		if( _advanced )
			_fuel = _fuel_max;

		if( _bFire )
		{
			_engineSound->Pause(false);
			_engineSound->MoveTo(_pos);

			_fuel = __max(0, _fuel - _fuel_rate * dt);
			if( 0 == _fuel ) _bReady = false;


			vec2d v = _proprietor->_lv;

			// основная струя
			{
				const float lenght = 50.0f;
				vec2d a(_proprietor->_angle + _angle);

				vec2d emitter = _pos - a * 20.0f;
				vec2d hit;
				GC_RigidBodyStatic *object = g_level->agTrace (
					g_level->grid_rigid_s, GetRawPtr(_proprietor), emitter, -a * lenght, &hit);
				if( object )
				{
					object->TakeDamage(dt * DAMAGE_RAM_ENGINE * (1.0f - (hit - emitter).Length() / lenght),
						hit, GetRawPtr(_proprietor));
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
				vec2d a(_proprietor->_angle + _angle + l * 0.15f);

				vec2d emitter = _pos - a * 15.0f + vec2d( -a.y, a.x) * l * 17.0f;
				vec2d hit;
				GC_RigidBodyStatic *object = g_level->agTrace(g_level->grid_rigid_s,
					GetRawPtr(_proprietor), emitter + a * 2.0f, -a * lenght, &hit);
				if( object )
				{
					object->TakeDamage(dt * DAMAGE_RAM_ENGINE * (1.0f - (hit - emitter).Length() / lenght),
						hit, GetRawPtr(_proprietor));
				}

				for( int i = 0; i < 10; i++ )
				{
					float time = frand(0.05f) + 0.02f;
					float t = (frand(1.0f) - 0.5f) * 2.5f;
					vec2d dx( -a.y * t, a.x * t);

					new GC_Particle(emitter + dx, v - a * (frand(600.0f)) - dx / time, tex3, time);
				}
			}

			float accel    = 500.0f * cosf(_angle);
			float maxspeed = 290.0f * cosf(_angle);
			_proprietor->_ForvAccel = 250 + __max(0,  accel);
			_proprietor->_BackAccel = 250 + __max(0, -accel);
			_proprietor->_MaxForvSpeed = 160.0f + __max(0,  maxspeed);
			_proprietor->_MaxBackSpeed = 160.0f + __max(0, -maxspeed);
//			_proprietor->_rotator.setl(5.5f, 30.0f, 90.0f);
		}
		else
		{
			_proprietor->_ForvAccel = 250;
			_proprietor->_BackAccel = 250;
			_proprietor->_MaxForvSpeed = 160;
			_proprietor->_MaxBackSpeed = 160;
//			_proprietor->_rotator.setl(3.5f, 10.0f, 30.0f);

			_engineSound->Pause(true);

			_fuel   = __min(_fuel_max, _fuel + _fuel_rep * dt);
			_bReady = (_fuel_max < _fuel * 4.0f);
		}

        _engineLight->Enable(_bFire);
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
	ED_ITEM( "weap_bfg", "Оружие:\tBig Fucking Gun" );
	return true;
}

GC_Weap_BFG::GC_Weap_BFG(float x, float y) : GC_Weapon(x, y)
{
	SetTexture("weap_bfg");
}

void GC_Weap_BFG::Attach(GC_Vehicle *pVehicle)
{
	GC_Weapon::Attach(pVehicle);

	_time_ready  = 0;
	_time_reload = 1.1f;

return;
	pVehicle->SetMaxHP(110);

	pVehicle->_ForvAccel = 250;
	pVehicle->_BackAccel = 200;
	pVehicle->_StopAccel = 1000;

//	pVehicle->_rotator.setl(3.5f, 15.0f, 30.0f);

	pVehicle->_MaxForvSpeed = 200;
	pVehicle->_MaxBackSpeed = 180;
}

GC_Weap_BFG::GC_Weap_BFG(FromFile) : GC_Weapon(FromFile())
{
}

GC_Weap_BFG::~GC_Weap_BFG()
{
}

GC_PickUp* GC_Weap_BFG::SetRespawn()
{
	return new GC_Weap_BFG(_pos.x, _pos.y);
}

void GC_Weap_BFG::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_Weapon::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_time_ready);
}

void GC_Weap_BFG::Fire()
{
	_ASSERT(_bAttached);

	if( _time >= _time_reload )
	{
		if( !_advanced && 0 == _time_ready )
		{
			PLAY(SND_BfgInit, _proprietor->_pos);
			_time_ready = FLT_EPSILON;
		}

		if( _time_ready >= 0.7f || _advanced )
		{
			vec2d a(_proprietor->_angle + _angle);

			float s = sinf(_proprietor->_angle + _angle);
			float c = cosf(_proprietor->_angle + _angle);

			new GC_BfgCore(_pos + a * 16.0f, vec2d(c, s) * SPEED_BFGCORE,
				GetRawPtr(_proprietor), _advanced );

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
	if( _bAttached && _time_ready != 0 )
	{
		_time_ready += dt;
		Fire();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Ripper)
{
	ED_ITEM( "weap_ripper", "Оружие:\tРипер" );
	return true;
}

void GC_Weap_Ripper::Attach(GC_Vehicle *pVehicle)
{
	GC_Weapon::Attach(pVehicle);

	_time_reload = 0.5f;

return;
	pVehicle->SetMaxHP(80);

	pVehicle->_ForvAccel = 300;
	pVehicle->_BackAccel = 200;
	pVehicle->_StopAccel = 500;

//	pVehicle->_rotator.setl(3.5f, 10.0f, 30.0f);

	pVehicle->_MaxBackSpeed = 260;
	pVehicle->_MaxForvSpeed = 240;
}

GC_Weap_Ripper::GC_Weap_Ripper(float x, float y)
: GC_Weapon(x, y)
{
	SetTexture("weap_ripper");
	new GC_Disk(this);
}

GC_Weap_Ripper::GC_Weap_Ripper(FromFile) : GC_Weapon(FromFile())
{
}

GC_Weap_Ripper::~GC_Weap_Ripper()
{
}

GC_PickUp* GC_Weap_Ripper::SetRespawn()
{
	return new GC_Weap_Ripper(_pos.x, _pos.y);
}

void GC_Weap_Ripper::Fire()
{
	if( _bAttached && _time >= _time_reload )
	{
		vec2d a(_proprietor->_angle + _angle);

		new GC_Disk(_pos - a * 9.0f, a * SPEED_DISK + net_vrand(10),
			GetRawPtr(_proprietor), _advanced );
		PLAY(SND_DiskFire, _pos);

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

void GC_Weap_Ripper::TimeStepFixed(float dt)
{
	GC_Weapon::TimeStepFixed( dt );

}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Minigun)
{
	ED_ITEM( "weap_minigun", "Оружие:\tПулемет" );
	return true;
}

void GC_Weap_Minigun::Attach(GC_Vehicle *pVehicle)
{
	GC_Weapon::Attach(pVehicle);

	_time_reload = 0.03f;
	_time_rotate = 0;
	_time_fire   = 0;
	_time_shot   = 0;

	_sound = new GC_Sound(SND_MinigunFire, SMODE_STOP, _pos);
	_bFire  = false;

	_fireEffect->SetTexture("minigun_fire");

return;
	pVehicle->SetMaxHP(65);

	pVehicle->_ForvAccel = 700;
	pVehicle->_BackAccel = 600;
	pVehicle->_StopAccel = 2000;

//	pVehicle->_rotator.setl(3.5f, 15.0f, 30.0f);

	pVehicle->_MaxBackSpeed = 300;
	pVehicle->_MaxForvSpeed = 350;

}

void GC_Weap_Minigun::Detach()
{
	SAFE_KILL(_crosshair_left);
	SAFE_KILL(_sound);

	GC_Weapon::Detach();
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

void GC_Weap_Minigun::SetCrosshair()
{
	_crosshair      = new GC_Crosshair(GC_Crosshair::CHS_DOUBLE);
	_crosshair_left = new GC_Crosshair(GC_Crosshair::CHS_DOUBLE);
}

GC_PickUp* GC_Weap_Minigun::SetRespawn()
{
	return new GC_Weap_Minigun(_pos.x, _pos.y);
}

void GC_Weap_Minigun::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_Weapon::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_bFire);
	f.Serialize(_time_fire);
	f.Serialize(_time_rotate);
	f.Serialize(_time_shot);
	/////////////////////////////////////
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
	_ASSERT(_bAttached);
	if( _bAttached )
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

	if( _crosshair_left && _proprietor )
	{
		_crosshair_left->Show(!g_options.players[_proprietor->_player->_nIndex].bAI);
	}

	if( _bAttached )
	{
		if( _bFire )
		{
			_time_rotate += dt;
			_time_shot   += dt;

			SetTexture((fmodf(_time_rotate, 0.08f) < 0.04f) ? "weap_mg1":"weap_mg2");

			_sound->MoveTo(_proprietor->_pos);
			_sound->Pause(false);
			_bFire = false;

			for(; _time_shot > 0; _time_shot -= _advanced ? 0.02f : 0.04f)
			{
				_time = frand(_feTime);
				_feOrient = frand(PI2);
				_fireEffect->Show(true);

				float da = _time_fire * 0.07f / WEAP_MG_TIME_RELAX;

				vec2d a(_proprietor->_angle + _angle + net_frand(da * 2.0f) - da);
				a *= (1 - net_frand(0.2f));

				new GC_Bullet(_proprietor->_pos + a * 18.0f, a * SPEED_BULLET,
					GetRawPtr(_proprietor), _advanced );

				if( !_advanced )
				{
					if( net_frand(WEAP_MG_TIME_RELAX * 5.0f) < _time_fire - WEAP_MG_TIME_RELAX * 0.2f )
					{
					//	_proprietor->_rotator.impulse(net_frand(4.0f) - 2.0f);
						float m = _proprietor->_inv_i;
						_proprietor->ApplyTorque(m * (net_frand(1.0f) - 0.5f));
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
	}

	///////////////////////////////////

	float da = _time_fire * 0.1f / WEAP_MG_TIME_RELAX;
	if( _crosshair )
	{
		_crosshair->_angle = _proprietor->_angle + da + _angle;
		_crosshair->MoveTo(_pos + vec2d(_crosshair->_angle) * CH_DISTANCE_THIN);
	}

	if( _crosshair_left )
	{
		_crosshair_left->_angle = _proprietor->_angle - da + _angle;
		_crosshair_left->MoveTo(_pos + vec2d(_crosshair_left->_angle) * CH_DISTANCE_THIN);
	}


	GC_Weapon::TimeStepFixed(dt);
}

///////////////////////////////////////////////////////////////////////////////
// end of file
