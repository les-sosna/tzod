// Vehicle.cpp

#include "stdafx.h"
#include "Vehicle.h"

#include "level.h"
#include "macros.h"
#include "options.h"
#include "functions.h"

#include "fs/SaveFile.h"

#include "GameClasses.h"
#include "particles.h"
#include "pickup.h"
#include "indicators.h"
#include "sound.h"
#include "player.h"
#include "turrets.h"

/////////////////////////////////////////////////////////////

GC_Vehicle::GC_Vehicle(GC_Player *pPlayer)
  : GC_RigidBodyDynamic(), _memberOf(g_level->vehicles, this) //, _rotator(_dir1)
{
	SetZ(Z_VEHICLES);

	_engine_power = 0;
	_rotate_power = 0;

	_time_smoke   = 0;

	ZeroMemory(&_state, sizeof(_state));

	new GC_IndicatorBar("indicator_health", this, &_health, &_health_max, LOCATION_TOP);
	_player = pPlayer;

	_light_ambient = new GC_Light(GC_Light::LIGHT_POINT);
	_light_ambient->SetIntensity(0.8f);
	_light_ambient->SetRadius(150);

	_light1 = new GC_Light(GC_Light::LIGHT_SPOT);
	_light2 = new GC_Light(GC_Light::LIGHT_SPOT);

	_light1->SetRadius(300);
	_light2->SetRadius(300);

	_light1->SetIntensity(0.9f);
	_light2->SetIntensity(0.9f);

	_light1->SetOffset(290);
	_light2->SetOffset(290);

	_light1->SetAspect(0.4f);
	_light2->SetAspect(0.4f);

	UpdateLight();

	_fTrackDensity = 8;
	_fTrackPathL   = 0;
	_fTrackPathR   = 0;

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING | GC_FLAG_OBJECT_EVENTS_TS_FIXED);
	SetShadow(true);
}

GC_Vehicle::GC_Vehicle(FromFile)
  : GC_RigidBodyDynamic(FromFile()), _memberOf(g_level->vehicles, this)//, _rotator(_dir1)
{
}

GC_Vehicle::~GC_Vehicle()
{
	_player = NULL;
}

void GC_Vehicle::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_RigidBodyDynamic::Serialize(f);
	/////////////////////////////////////
//	_rotator.Serialize(f);
	/////////////////////////////////////
	f.Serialize(_engine_power);
	f.Serialize(_rotate_power);
	f.Serialize(_BackAccel);
//	f.Serialize(_dir1);
	f.Serialize(_ForvAccel);
	f.Serialize(_MaxBackSpeed);
	f.Serialize(_MaxForvSpeed);
	f.Serialize(_state);
	f.Serialize(_StopAccel);
	f.Serialize(_time_smoke);
	f.Serialize(_fTrackDensity);
	f.Serialize(_fTrackPathL);
	f.Serialize(_fTrackPathR);
	/////////////////////////////////////
	f.Serialize(_damLabel);
	f.Serialize(_light_ambient);
	f.Serialize(_light1);
	f.Serialize(_light2);
	f.Serialize(_moveSound);
	f.Serialize(_player);
	f.Serialize(_weapon);
}

void GC_Vehicle::Kill()
{
	// _player освобождается в деструкторе

	SAFE_KILL(_damLabel);
	SAFE_KILL(_moveSound);
	SAFE_KILL(_light_ambient);
	SAFE_KILL(_light1);
	SAFE_KILL(_light2);

	if( _weapon )
	{
		_weapon->_bRespawn = false;
		_weapon->Detach();
		_weapon = NULL;
	}

	GC_RigidBodyDynamic::Kill();
}

void GC_Vehicle::SetState(VehicleState *pState)
{
	memcpy( &_state, pState, sizeof(VehicleState) );
}

void GC_Vehicle::SetSkin(const char *pSkinName)
{
	string_t tmp = "skin/";
	tmp += pSkinName;
	SetTexture(tmp.c_str());
	CenterPivot();
}

void GC_Vehicle::SetClass(const VehicleClass &vc)
{
	float max_r = 0;
	for( int i = 0; i < 4; i++ )
	{
		_vertices[i] = vc.bounds[i];
		if( _vertices[i].Length() > max_r )
			max_r = _vertices[i].Length();
	}

	_hsize.Set(max_r, max_r);


	_inv_m  = 1.0f / vc.m;
	_inv_i  = 1.0f / vc.i;

	_Nx     = vc._Nx;
	_Ny     = vc._Ny;
	_Nw     = vc._Nw;

	_Mx     = vc._Mx;
	_My     = vc._My;
	_Mw     = vc._Mw;

	_percussion = vc.percussion;
	_fragility  = vc.fragility;

	_engine_power = vc.engine_power;
	_rotate_power = vc.rotate_power;

	SetMaxHP(vc.health);
}

bool GC_Vehicle::TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from)
{
	_ASSERT(!IsKilled());

	DAMAGEDESC dd;
	dd.damage = damage;
	dd.hit    = hit;
	dd.from   = from;

	PulseNotify(NOTIFY_DAMAGE_FILTER, &dd);
    if( 0 == dd.damage ) return false;

	SetHealthCur(GetHealth() - dd.damage);

	if( OPT(bDamageLabel) )
	{
		if( _damLabel )
			_damLabel->Reset();
		else
			_damLabel = new GC_DamLabel(this);
	}

	ENUM_BEGIN(cameras, GC_Camera, pCamera)
	{
		if( !pCamera->_player ) continue;
		if( this == pCamera->_player->_vehicle )
		{
			pCamera->Shake(GetHealth() <= 0 ? 2.0f : dd.damage / GetHealthMax());
			break;
		}
	} ENUM_END();


	if( GetHealth() <= 0 )
	{
		char msg[256];
		char score[8];
		char *font = NULL;

		GC_Vehicle *pVehicle = dynamic_cast<GC_Vehicle *>(dd.from);
		if( NULL != pVehicle )
		{
			if( pVehicle->_player == _player )
			{
				// убил себя апстену =)
				pVehicle->_player->_score--;
				font = "font_digits_red";
				sprintf(msg, "%s совершил самоубийство", pVehicle->_player->_name);
			}
			else
			{
				if( 0 != _player->_team &&
					((GC_Vehicle *) dd.from)->_player->_team == _player->_team)
				{
					// убил товарища
					pVehicle->_player->_score--;
					font = "font_digits_red";
					wsprintf(msg, "нехороший %s замочил своего друга %s",
						((GC_Vehicle *) dd.from)->_player->_name, _player->_name);
				}
				else
				{
					// убил врага - молодец!
					++pVehicle->_player->_score;
					font = "font_digits_green";
					wsprintf(msg, "%s замочил своего врага %s",
						((GC_Vehicle *) dd.from)->_player->_name, _player->_name);
				}

				if( OPT(fraglimit) )
				{
					if( pVehicle->_player->_score >= OPT(fraglimit) )
					{
						g_level->Pause(true);
						g_level->_limitHit = true;
					}
				}
			}

			if( !pVehicle->_player->dead() )
			{
				wsprintf(score, "%d", pVehicle->_player->_score);
				new GC_Text_ToolTip(pVehicle->_pos, score, font);
			}
		}
		else
		if( dynamic_cast<GC_Turret *>(dd.from) )
		{
			// убийца - стационарная установка

			_player->_score--;
			wsprintf(score, "%d", _player->_score);
			new GC_Text_ToolTip(_pos, score, "font_digits_red");
			wsprintf(msg, "%s нарвался на неприятности", _player->_name);
		}
		else
		{
			wsprintf(msg, "c %s случился несчастный случай", _player->_name);
		}

		OnDestroy();
		Kill();
		_MessageArea::Inst()->message(msg);
		return true;
	}
	return false;
}

void GC_Vehicle::SetMaxHP(float hp)
{
	SetHealth(hp * GetHealth() / GetHealthMax(), hp);
}

void GC_Vehicle::TimeStepFixed(float dt)
{
	static const TextureCache smoke("particle_smoke");
	static const TextureCache track("cat_track");


	//
	// запоминаем положение гусениц
	//

	vec2d trackTmp(_angle+PI/2);
	vec2d trackL = _pos + trackTmp*15;
	vec2d trackR = _pos - trackTmp*15;



	//
	// spawn damage smoke
	//
	if( OPT(bParticles) && GetHealth() < (GetHealthMax() * 0.4f) )
	{
		_ASSERT(GetHealth() > 0);
							//    +-{ максимальное число частичек дыма в секунду }
		_time_smoke += dt;	//    |
		float smoke_dt = 1.0f / (60.0f * (1.0f - GetHealth() / (GetHealthMax() * 0.5f)));
		for(; _time_smoke > 0; _time_smoke -= smoke_dt)
		{
			(new GC_Particle(_pos + vrand(frand(24.0f)), SPEED_SMOKE,
				smoke, 1.5f))->_time = frand(1.0f);
		}
	}

	// move...
	GC_RigidBodyDynamic::TimeStepFixed( dt );
	if( IsKilled() ) return;


	//
	// adjast speed
	//

	if( _state._bState_MoveForvard )
		ApplyForce(_direction * _engine_power);
	else
	if( _state._bState_MoveBack )
		ApplyForce(-_direction * _engine_power);



	if( _state._bExplicitBody )
	{
		//
		// выбираем направление
		//

		float target = fmodf(_state._fBodyAngle, PI2);

		float xt1 = target - PI2;
		float xt2 = target + PI2;

		if( fabsf(_angle - xt1) < fabsf(_angle - xt2) &&
			fabsf(_angle - xt1) < fabsf(_angle - target) )
		{
			ApplyMomentum( -_rotate_power );
		}
		else
		if( fabsf(_angle - xt2) < fabsf(_angle - xt1) &&
			fabsf(_angle - xt2) < fabsf(_angle - target) )
		{
			ApplyMomentum(  _rotate_power );
		}
		else
		{
			if( target > _angle )
			{
				ApplyMomentum(  _rotate_power );
			}
			else
			{
				ApplyMomentum( -_rotate_power );
			}
		}
	}
	else
	{
		if( _state._bState_RotateLeft )
			ApplyMomentum( -_rotate_power );
		else
		if( _state._bState_RotateRight )
			ApplyMomentum(  _rotate_power );
	}



    UpdateLight();

	if( _moveSound && !(g_options.bModeEditor || g_level->_limitHit) )
	{
		_moveSound->MoveTo(_pos);
		_moveSound->SetSpeed (__min(1, 0.5f + 0.5f * _lv.Length() / _MaxForvSpeed));
		_moveSound->SetVolume(__min(1, 0.9f + 0.1f * _lv.Length() / _MaxForvSpeed));
	}


	//
	// caterpillar tracks
	//

	vec2d tmp(_angle+PI/2);
	vec2d trackL_new = _pos + tmp*15;
	vec2d trackR_new = _pos - tmp*15;

	vec2d e = trackL_new - trackL;
	float len = e.Length();
	e /= len;
	while( _fTrackPathL < len )
	{
		if( g_options.bParticles )
		{
			GC_Particle *p = new GC_Particle(trackL + e * _fTrackPathL,
				vec2d(0, 0), track, 12, e.Angle());
			p->SetZ(Z_WATER);
			p->SetFade(true);
		}
		_fTrackPathL += _fTrackDensity;
	}
	_fTrackPathL -= len;

	e   = trackR_new - trackR;
	len = e.Length();
	e  /= len;
	while( _fTrackPathR < len )
	{
		if( g_options.bParticles )
		{
			GC_Particle *p = new GC_Particle(trackR + e * _fTrackPathR,
				vec2d(0, 0), track, 12, e.Angle());
			p->SetZ(Z_WATER);
			p->SetFade(true);
		}
		_fTrackPathR += _fTrackDensity;
	}
	_fTrackPathR -= len;



	//
	// die if out of level bounds
	//
	if( _pos.x < 0 || _pos.x > g_level->_sx ||
		_pos.y < 0 || _pos.y > g_level->_sy )
	{
		if( !TakeDamage(GetHealth(), _pos, this) ) Kill();
	}
}

void GC_Vehicle::UpdateLight()
{
	_light1->MoveTo(_pos + vec2d(_angle + 0.6f) * 20 );
	_light1->SetAngle(_angle + 0.2f);
	_light1->Enable(_state._bLight);
	_light2->MoveTo(_pos + vec2d(_angle - 0.6f) * 20 );
	_light2->SetAngle(_angle - 0.2f);
	_light2->Enable(_state._bLight);
	_light_ambient->MoveTo(_pos);
	_light_ambient->Enable(_state._bLight);
}

void GC_Vehicle::Draw()
{
	SetRotation(_angle);
	GC_RigidBodyDynamic::Draw();
}

void GC_Vehicle::SetMoveSound(enumSoundTemplate s)
{
	_moveSound = new GC_Sound(s, SMODE_LOOP, _pos);
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Tank_Light)
{
	return true;
}

GC_Tank_Light::GC_Tank_Light(vec2d pos, float dir, GC_Player *pPlayer) : GC_Vehicle(pPlayer)
{
	_direction = vec2d(_angle = dir);

	_MaxBackSpeed = 150;
	_MaxForvSpeed = 200;

	MoveTo(pos);
	SetMoveSound(SND_TankMove);
}

GC_Tank_Light::GC_Tank_Light(FromFile) : GC_Vehicle(FromFile())
{
}

/*
void GC_Tank_Light::SetDefaults()
{
	SetClass(_player->_class);

	_MaxBackSpeed = 150;
	_MaxForvSpeed = 200;


	return;

	_hsize.Set(25, 25);

	_vertices[0].Set( 18.5f,  18.5f);
	_vertices[1].Set(-18.5f,  18.5f);
	_vertices[2].Set(-18.5f, -18.5f);
	_vertices[3].Set( 18.5f, -18.5f);

	_inv_m  = 1;

	_Nx     = 100;
	_Ny     = 1000;
	_Nw     = 50;

	_Mx     = 0.5f;
	_My     = 2.5f;
	_Mw     = 1;

	_percussion = 1;

	_engine_power = 100000;
	_rotate_power = 50000;

	_ForvAccel = 500;
	_BackAccel = 200;
	_StopAccel = 500;

//	_rotator.reset(_angle, _rotator.getv(), 3.5f, 10.0f, 30.0f);

	SetMaxHP(GetDefaultHealth());
}
*/

void GC_Tank_Light::OnDestroy()
{
	new GC_Boom_Big( _pos, NULL);
}

// end of file
