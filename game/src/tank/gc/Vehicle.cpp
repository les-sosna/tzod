// Vehicle.cpp

#include "stdafx.h"
#include "Vehicle.h"

#include "level.h"
#include "macros.h"
#include "functions.h"

#include "fs/SaveFile.h"

#include "GameClasses.h"
#include "Camera.h"
#include "particles.h"
#include "pickup.h"
#include "indicators.h"
#include "sound.h"
#include "player.h"
#include "turrets.h"
#include "Weapons.h"

#include "config/Config.h"

#include "core/Console.h"

#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"
#include "ui/gui.h"


///////////////////////////////////////////////////////////////////////////////

GC_Vehicle::GC_Vehicle(float x, float y)
  : GC_RigidBodyDynamic()
  , _memberOf(this)
//  , _rotator(_dir1)
{
	SetZ(Z_VEHICLES);

	_enginePower = 0;
	_rotatePower = 0;
	_maxRotSpeed = 0;
	_maxLinSpeed = 0;

	_time_smoke   = 0;

	ZeroMemory(&_state, sizeof(_state));

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

	_trackDensity = 8;
	_trackPathL   = 0;
	_trackPathR   = 0;

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING | GC_FLAG_OBJECT_EVENTS_TS_FIXED);
	SetShadow(true);

	MoveTo(vec2d(x, y));
}

GC_Vehicle::GC_Vehicle(FromFile)
  : GC_RigidBodyDynamic(FromFile())
  , _memberOf(this)
//  , _rotator(_dir1)
{
}

GC_Vehicle::~GC_Vehicle()
{
	_player = NULL;
}

void GC_Vehicle::SetPlayer(GC_Player *player)
{
	new GC_IndicatorBar("indicator_health", this, &_health, &_health_max, LOCATION_TOP);
	_player = player;
}

void GC_Vehicle::Serialize(SaveFile &f)
{
	GC_RigidBodyDynamic::Serialize(f);

	f.Serialize(_enginePower);
	f.Serialize(_rotatePower);
	f.Serialize(_maxRotSpeed);
	f.Serialize(_maxLinSpeed);
	f.Serialize(_state);
	f.Serialize(_time_smoke);
	f.Serialize(_trackDensity);
	f.Serialize(_trackPathL);
	f.Serialize(_trackPathR);
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
		_weapon->SetRespawn(true);
		_weapon->Detach();
	}

	GC_RigidBodyDynamic::Kill();
}

void GC_Vehicle::OnPickup(GC_Pickup *pickup, bool attached)
{
	GC_RigidBodyDynamic::OnPickup(pickup, attached);
	if( GC_Weapon *w = dynamic_cast<GC_Weapon *>(pickup) )
	{
		if( attached )
		{
			if( _weapon )
			{
				_weapon->Disappear(); // this will detach weapon and call OnPickup(attached=false)
			}

			_ASSERT(!_weapon);
			_weapon = w;

			//
			// update class
			//

			VehicleClass vc;

			lua_State *L = g_env.L;
			lua_pushcfunction(L, luaT_ConvertVehicleClass); // function to call
			lua_getglobal(L, "getvclass");
			lua_pushstring(L, GetPlayer()->GetClass().c_str());  // cls arg
			lua_pushstring(L, g_level->GetTypeName(_weapon->GetType()));  // weap arg
			if( lua_pcall(L, 2, 1, 0) )
			{
				// print error message
				g_console->printf("%s\n", lua_tostring(L, -1));
				lua_pop(L, 1);
				return;
			}

			lua_pushlightuserdata(L, &vc);
			if( lua_pcall(L, 2, 0, 0) )
			{
				// print error message
				g_console->printf("%s\n", lua_tostring(L, -1));
				lua_pop(L, 1);
				return;
			}

			SetClass(vc);
		}
		else
		{
			_ASSERT(_weapon);
			Unsubscribe( GetRawPtr(_weapon) );
			_weapon = NULL;
			ResetClass();
		}
	}
}

void GC_Vehicle::SetState(const VehicleState &vs)
{
	memcpy( &_state, &vs, sizeof(VehicleState) );
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
		if( _vertices[i].len() > max_r )
			max_r = _vertices[i].len();
	}

	_radius = max_r;


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

	_enginePower = vc.enginePower;
	_rotatePower = vc.rotatePower;

	_maxLinSpeed = vc.maxLinSpeed;
	_maxRotSpeed = vc.maxRotSpeed;

	SetMaxHP(vc.health);
}

void GC_Vehicle::ResetClass()
{
	lua_State *L = g_env.L;
	lua_pushcfunction(L, luaT_ConvertVehicleClass); // function to call

	lua_getglobal(L, "getvclass");
	lua_pushstring(L, GetPlayer()->GetClass().c_str()); // cls arg
	if( lua_pcall(L, 1, 1, 0) )  // call getvclass(clsname)
	{
		// print error message
		g_console->printf("%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	VehicleClass vc;
	lua_pushlightuserdata(L, &vc);
	if( lua_pcall(L, 2, 0, 0) )
	{
		// print error message
		g_console->printf("%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	SetClass(vc);
}

bool GC_Vehicle::TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from)
{
	_ASSERT(!IsKilled());

	DamageDesc dd;
	dd.damage = damage;
	dd.hit    = hit;
	dd.from   = from;

	PulseNotify(NOTIFY_DAMAGE_FILTER, &dd);
    if( 0 == dd.damage ) return false;

	SetHealthCur(GetHealth() - dd.damage);

	if( g_conf.g_showdamage->Get() )
	{
		if( _damLabel )
			_damLabel->Reset();
		else
			_damLabel = new GC_DamLabel(this);
	}

	FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
	{
		if( !pCamera->_player ) continue;
		if( this == pCamera->_player->GetVehicle() )
		{
			pCamera->Shake(GetHealth() <= 0 ? 2.0f : dd.damage / GetHealthMax());
			break;
		}
	}

	if( GetHealth() <= 0 )
	{
		char msg[256] = {0};
		char score[8];
		char *font = NULL;

		if( GC_Vehicle *veh = dynamic_cast<GC_Vehicle *>(dd.from) )
		{
			if( veh->GetPlayer() == GetPlayer() )
			{
				// убил себя апстену =)
				veh->GetPlayer()->SetScore(veh->GetPlayer()->GetScore() - 1);
				font = "font_digits_red";
				wsprintf( msg, "%s совершил самоубийство", veh->GetPlayer()->GetNick().c_str() );
			}
			else if( GetPlayer() )
			{
				if( 0 != GetPlayer()->GetTeam() &&
					veh->GetPlayer()->GetTeam() == GetPlayer()->GetTeam() )
				{
					// убил товарища
					veh->GetPlayer()->SetScore(veh->GetPlayer()->GetScore() - 1);
					font = "font_digits_red";
					wsprintf( msg, "нехороший %s замочил своего друга %s",
						((GC_Vehicle *) dd.from)->GetPlayer()->GetNick().c_str(), 
						GetPlayer()->GetNick().c_str() );
				}
				else
				{
					// убил врага - молодец!
					veh->GetPlayer()->SetScore(veh->GetPlayer()->GetScore() + 1);
					font = "font_digits_green";
					wsprintf( msg, "%s замочил своего врага %s",
						veh->GetPlayer()->GetNick().c_str(), GetPlayer()->GetNick().c_str() );
				}
			}
			else
			{
				// убил бездушный танк
				veh->GetPlayer()->SetScore(veh->GetPlayer()->GetScore() + 1);
				font = "font_digits_green";
			}

			if( !veh->GetPlayer()->IsDead() )
			{
				wsprintf( score, "%d", veh->GetPlayer()->GetScore() );
				new GC_Text_ToolTip(veh->GetPos(), score, font);
			}
		}
		else
		if( dynamic_cast<GC_Turret *>(dd.from) )
		{
			// убийца - стационарная установка
			GetPlayer()->SetScore(GetPlayer()->GetScore() - 1);
			wsprintf(score, "%d", GetPlayer()->GetScore());
			new GC_Text_ToolTip(GetPos(), score, "font_digits_red");
			wsprintf(msg, "%s нарвался на неприятности", GetPlayer()->GetNick().c_str());
		}
		else
		{
			wsprintf(msg, "c %s случился несчастный случай", GetPlayer()->GetNick().c_str());
		}

		AddRef();
			OnDestroy();
			Kill();
		Release();

		static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->puts(msg);
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
	vec2d trackL = GetPos() + trackTmp*15;
	vec2d trackR = GetPos() - trackTmp*15;



	//
	// spawn damage smoke
	//
	if( GetHealth() < (GetHealthMax() * 0.4f) )
	{
		_ASSERT(GetHealth() > 0);
		                    //    +-{ максимальное число частичек дыма в секунду }
		_time_smoke += dt;  //    |
		float smoke_dt = 1.0f / (60.0f * (1.0f - GetHealth() / (GetHealthMax() * 0.5f)));
		for(; _time_smoke > 0; _time_smoke -= smoke_dt)
		{
			(new GC_Particle(GetPos() + vrand(frand(24.0f)), SPEED_SMOKE,
				smoke, 1.5f))->_time = frand(1.0f);
		}
	}

	// move...
	GC_RigidBodyDynamic::TimeStepFixed( dt );
	if( IsKilled() ) return;

	// fire...
	if( _weapon && _state._bState_Fire )
	{
		_weapon->Fire();
		if( IsKilled() ) return;
	}


	//
	// adjust speed
	//

	if( _state._bState_MoveForward )
	{
		ApplyForce(_direction * _enginePower);
	}
	else
	if( _state._bState_MoveBack )
	{
		ApplyForce(-_direction * _enginePower);
	}



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
			ApplyMomentum( -_rotatePower / _inv_i );
		}
		else
		if( fabsf(_angle - xt2) < fabsf(_angle - xt1) &&
			fabsf(_angle - xt2) < fabsf(_angle - target) )
		{
			ApplyMomentum(  _rotatePower / _inv_i );
		}
		else
		{
			if( target > _angle )
			{
				ApplyMomentum(  _rotatePower / _inv_i );
			}
			else
			{
				ApplyMomentum( -_rotatePower / _inv_i );
			}
		}
	}
	else
	{
		if( fabsf(_av) < _maxRotSpeed )
		{
			if( _state._bState_RotateLeft )
				ApplyMomentum( -_rotatePower / _inv_i );
			else
			if( _state._bState_RotateRight )
				ApplyMomentum(  _rotatePower / _inv_i );
		}
	}



	UpdateLight();

	if( _moveSound && !(g_level->_modeEditor || g_level->_limitHit) )
	{
		_moveSound->MoveTo(GetPos());
		_moveSound->SetSpeed (__min(1, 0.5f + 0.5f * _lv.len() / GetMaxSpeed()));
		_moveSound->SetVolume(__min(1, 0.9f + 0.1f * _lv.len() / GetMaxSpeed()));
	}


	//
	// caterpillar tracks
	//

	if( g_conf.g_particles->Get() )
	{
		vec2d tmp(_angle+PI/2);
		vec2d trackL_new = GetPos() + tmp*15;
		vec2d trackR_new = GetPos() - tmp*15;

		vec2d e = trackL_new - trackL;
		float len = e.len();
		e /= len;
		while( _trackPathL < len )
		{
			GC_Particle *p = new GC_Particle(trackL + e * _trackPathL,
				vec2d(0, 0), track, 12, e.Angle());
			p->SetZ(Z_WATER);
			p->SetFade(true);
			_trackPathL += _trackDensity;
		}
		_trackPathL -= len;

		e   = trackR_new - trackR;
		len = e.len();
		e  /= len;
		while( _trackPathR < len )
		{
			GC_Particle *p = new GC_Particle(trackR + e * _trackPathR,
				vec2d(0, 0), track, 12, e.Angle());
			p->SetZ(Z_WATER);
			p->SetFade(true);
			_trackPathR += _trackDensity;
		}
		_trackPathR -= len;
	}


	//
	// die if out of level bounds
	//
	if( GetPos().x < 0 || GetPos().x > g_level->_sx ||
		GetPos().y < 0 || GetPos().y > g_level->_sy )
	{
		if( !TakeDamage(GetHealth(), GetPos(), this) ) Kill();
	}
}

void GC_Vehicle::UpdateLight()
{
	_light1->MoveTo(GetPos() + vec2d(_angle + 0.6f) * 20 );
	_light1->SetAngle(_angle + 0.2f);
	_light1->Activate(_state._bLight);
	_light2->MoveTo(GetPos() + vec2d(_angle - 0.6f) * 20 );
	_light2->SetAngle(_angle - 0.2f);
	_light2->Activate(_state._bLight);
	_light_ambient->MoveTo(GetPos());
	_light_ambient->Activate(_state._bLight);
}

void GC_Vehicle::Draw()
{
	SetRotation(_angle);
	GC_RigidBodyDynamic::Draw();
}

void GC_Vehicle::SetMoveSound(enumSoundTemplate s)
{
	_moveSound = new GC_Sound(s, SMODE_LOOP, GetPos());
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Tank_Light)
{
	ED_ACTOR("tank", "Объект: Танк", 1, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, 0);
	return true;
}

GC_Tank_Light::GC_Tank_Light(float x, float y)
  : GC_Vehicle(x, y)
{
//	_MaxBackSpeed = 150;
//	_MaxForvSpeed = 200;

	SetMoveSound(SND_TankMove);
	SetSkin("red");
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

	_enginePower = 100000;
	_rotatePower = 50000;

	_ForvAccel = 500;
	_BackAccel = 200;
	_StopAccel = 500;

//	_rotator.reset(_angle, _rotator.getv(), 3.5f, 10.0f, 30.0f);

	SetMaxHP(GetDefaultHealth());
}
*/

void GC_Tank_Light::OnDestroy()
{
	new GC_Boom_Big( GetPos(), NULL);
	__super::OnDestroy();
}

// end of file
