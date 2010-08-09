// Vehicle.cpp

#include "stdafx.h"
#include "Vehicle.h"

#include "level.h"
#include "macros.h"
#include "functions.h"
#include "script.h"

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
#include "config/Language.h"

#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"
#include "ui/gui.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_VehicleVisualDummy)
{
	return true;
}

GC_VehicleVisualDummy::GC_VehicleVisualDummy(GC_Vehicle *parent)
  : GC_VehicleBase()
  , _time_smoke(0)
  , _trackDensity(8)
  , _trackPathL(0)
  , _trackPathR(0)
  , _parent(parent)
{
	SetFlags(GC_FLAG_RBSTATIC_PHANTOM|GC_FLAG_VEHICLEDUMMY_TRACKS, true);
	SetZ(Z_VEHICLES);
	SetShadow(true);

	_parent->Subscribe(NOTIFY_DAMAGE_FILTER, this, (NOTIFYPROC) &GC_VehicleVisualDummy::OnDamageParent);

	_light_ambient = WrapRawPtr(new GC_Light(GC_Light::LIGHT_POINT));
	_light_ambient->SetIntensity(0.8f);
	_light_ambient->SetRadius(150);

	_light1 = WrapRawPtr(new GC_Light(GC_Light::LIGHT_SPOT));
	_light2 = WrapRawPtr(new GC_Light(GC_Light::LIGHT_SPOT));

	_light1->SetRadius(300);
	_light2->SetRadius(300);

	_light1->SetIntensity(0.9f);
	_light2->SetIntensity(0.9f);

	_light1->SetOffset(290);
	_light2->SetOffset(290);

	_light1->SetAspect(0.4f);
	_light2->SetAspect(0.4f);

	// time step fixed is called by player
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING /*| GC_FLAG_OBJECT_EVENTS_TS_FIXED*/);

	MoveTo(_parent->GetPos());
	UpdateLight();
}

GC_VehicleVisualDummy::GC_VehicleVisualDummy(FromFile)
  : GC_VehicleBase(FromFile())
{
}

GC_VehicleVisualDummy::~GC_VehicleVisualDummy()
{
}

bool GC_VehicleVisualDummy::TakeDamage(float damage, const vec2d &hit, GC_Player *from)
{
	assert(false);
	return false;
}

void GC_VehicleVisualDummy::Kill()
{
	SAFE_KILL(_damLabel);
	SAFE_KILL(_moveSound);
	SAFE_KILL(_light_ambient);
	SAFE_KILL(_light1);
	SAFE_KILL(_light2);
	_parent = NULL;
	GC_VehicleBase::Kill();
}

void GC_VehicleVisualDummy::Serialize(SaveFile &f)
{
	GC_VehicleBase::Serialize(f);

	f.Serialize(_time_smoke);
	f.Serialize(_trackDensity);
	f.Serialize(_trackPathL);
	f.Serialize(_trackPathR);
	f.Serialize(_damLabel);
	f.Serialize(_light_ambient);
	f.Serialize(_light1);
	f.Serialize(_light2);
	f.Serialize(_moveSound);
	f.Serialize(_parent);
}

void GC_VehicleVisualDummy::Draw() const
{
	GC_VehicleBase::Draw();

	if( g_conf.g_shownames.Get() && _parent->GetOwner() )
	{
		const vec2d &pos = GetPosPredicted();
		static TextureCache f("font_small");
		g_texman->DrawBitmapText(floorf(pos.x), floorf(pos.y + GetSpriteHeight()/2),
			f.GetTexture(), 0x7f7f7f7f, _parent->GetOwner()->GetNick(), alignTextCT);
	}
}

void GC_VehicleVisualDummy::TimeStepFixed(float dt)
{
	static const TextureCache track("cat_track");

	if( _moveSound && !(g_level->GetEditorMode() || g_level->_limitHit) )
	{
		_moveSound->MoveTo(GetPos());
		float v = _lv.len() / _parent->GetMaxSpeed();
		_moveSound->SetSpeed (__min(1, 0.5f + 0.5f * v));
		_moveSound->SetVolume(__min(1, 0.9f + 0.1f * v));
	}


	//
	// remember position
	//

	vec2d trackTmp(GetDirection().y, -GetDirection().x);
	vec2d trackL = GetPos() + trackTmp*15;
	vec2d trackR = GetPos() - trackTmp*15;


	// move
	GC_VehicleBase::TimeStepFixed( dt );
	assert(!IsKilled());

	ApplyState(_parent->GetPredictedState());


	//
	// caterpillar tracks
	//

	if( g_conf.g_particles.Get() && CheckFlags(GC_FLAG_VEHICLEDUMMY_TRACKS) )
	{
		vec2d tmp(GetDirection().y, -GetDirection().x);
		vec2d trackL_new = GetPos() + tmp*15;
		vec2d trackR_new = GetPos() - tmp*15;

		vec2d e = trackL_new - trackL;
		float len = e.len();
		e /= len;
		while( _trackPathL < len )
		{
			GC_Particle *p = new GC_Particle(trackL + e * _trackPathL, vec2d(0,0), track, 12, e);
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
			GC_Particle *p = new GC_Particle(trackR + e * _trackPathR, vec2d(0, 0), track, 12, e);
			p->SetZ(Z_WATER);
			p->SetFade(true);
			_trackPathR += _trackDensity;
		}
		_trackPathR -= len;
	}

	UpdateLight();
}

void GC_VehicleVisualDummy::TimeStepFloat(float dt)
{
	static const TextureCache smoke("particle_smoke");

	//
	// spawn damage smoke
	//
	if( _parent->GetHealth() < (_parent->GetHealthMax() * 0.4f) )
	{
		assert(!_parent->IsKilled());
		assert(_parent->GetHealth() > 0);
		                    //    +-{ particles per second }
		_time_smoke += dt;  //    |
		float smoke_dt = 1.0f / (60.0f * (1.0f - _parent->GetHealth() / (_parent->GetHealthMax() * 0.5f)));
		for(; _time_smoke > 0; _time_smoke -= smoke_dt)
		{
			(new GC_Particle(GetPos() + vrand(frand(24.0f)), SPEED_SMOKE, smoke, 1.5f))->_time = frand(1.0f);
		}
	}


	GC_VehicleBase::TimeStepFloat(dt);
}

void GC_VehicleVisualDummy::SetMoveSound(enumSoundTemplate s)
{
	_moveSound = WrapRawPtr(new GC_Sound(s, SMODE_LOOP, GetPos()));
}

void GC_VehicleVisualDummy::UpdateLight()
{
	static const vec2d delta1(0.6f);
	static const vec2d delta2(-0.6f);
	_light1->MoveTo(GetPos() + Vec2dAddDirection(GetDirection(), delta1) * 20 );
	_light1->SetLightDirection(GetDirection());
	_light1->SetActive(_parent->GetPredictedState()._bLight);
	_light2->MoveTo(GetPos() + Vec2dAddDirection(GetDirection(), delta2) * 20 );
	_light2->SetLightDirection(GetDirection());
	_light2->SetActive(_parent->GetPredictedState()._bLight);
	_light_ambient->MoveTo(GetPos());
	_light_ambient->SetActive(_parent->GetPredictedState()._bLight);
}

void GC_VehicleVisualDummy::OnDamageParent(GC_Object *sender, void *param)
{
	if( g_conf.g_showdamage.Get() )
	{
		if( _damLabel )
		{
			_damLabel->Reset();
		}
		else
		{
			_damLabel = WrapRawPtr(new GC_DamLabel(this));
			_damLabel->Subscribe(NOTIFY_OBJECT_KILL, this, 
				(NOTIFYPROC) &GC_VehicleVisualDummy::OnDamLabelDisappear);
		}
	}
}

void GC_VehicleVisualDummy::OnDamLabelDisappear(GC_Object *sender, void *param)
{
	assert(_damLabel);
	_damLabel = NULL;
}

///////////////////////////////////////////////////////////////////////////////

GC_VehicleBase::GC_VehicleBase()
  : GC_RigidBodyDynamic()
  , _enginePower(0)
  , _rotatePower(0)
  , _maxRotSpeed(0)
  , _maxLinSpeed(0)
{
}

GC_VehicleBase::GC_VehicleBase(FromFile)
  : GC_RigidBodyDynamic(FromFile())
{
}

GC_VehicleBase::~GC_VehicleBase()
{
}

void GC_VehicleBase::SetClass(const VehicleClass &vc)
{
	for( int i = 0; i < 4; i++ )
	{
		SetSize(vc.width, vc.length);
	}

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

void GC_VehicleBase::SetMaxHP(float hp)
{
	SetHealth(hp * GetHealth() / GetHealthMax(), hp);
}

void GC_VehicleBase::Serialize(SaveFile &f)
{
	GC_RigidBodyDynamic::Serialize(f);

	f.Serialize(_enginePower);
	f.Serialize(_rotatePower);
	f.Serialize(_maxRotSpeed);
	f.Serialize(_maxLinSpeed);
}

void GC_VehicleBase::ApplyState(const VehicleState &vs)
{
	//
	// adjust speed
	//

	if( vs._bState_MoveForward )
	{
		ApplyForce(GetDirection() * _enginePower);
	}
	else
	if( vs._bState_MoveBack )
	{
		ApplyForce(-GetDirection() * _enginePower);
	}


	if( vs._bExplicitBody )
	{
		if( Vec2dCross(GetDirection(), vec2d(vs._fBodyAngle - GetSpinup())) < 0 && -_av < _maxRotSpeed )
			ApplyMomentum( -_rotatePower / _inv_i );
		else if( _av < _maxRotSpeed )
			ApplyMomentum( _rotatePower / _inv_i );
	}
	else
	{
		if( vs._bState_RotateLeft && -_av < _maxRotSpeed )
			ApplyMomentum( -_rotatePower / _inv_i );
		else if( vs._bState_RotateRight && _av < _maxRotSpeed )
			ApplyMomentum(  _rotatePower / _inv_i );
	}
}

///////////////////////////////////////////////////////////////////////////////

GC_Vehicle::GC_Vehicle(float x, float y)
  : GC_VehicleBase()
  , _memberOf(this)
{
	ZeroMemory(&_stateReal, sizeof(VehicleState));

	MoveTo(vec2d(x, y));

	_visual = WrapRawPtr(new GC_VehicleVisualDummy(this));

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
#ifdef _DEBUG
	SetZ(Z_VEHICLES);
#endif
}

GC_Vehicle::GC_Vehicle(FromFile)
  : GC_VehicleBase(FromFile())
  , _memberOf(this)
{
}

GC_Vehicle::~GC_Vehicle()
{
}

float GC_Vehicle::GetMaxSpeed() const
{
	return __min((_enginePower - _Nx) / _Mx, _maxLinSpeed);
}

float GC_Vehicle::GetMaxBrakingLength() const
{
	float result;
	
	float vx = GetMaxSpeed();
	assert(vx > 0);

	if( _Mx > 0 )
	{
		if( _Nx > 0 )
		{
			result = vx/_Mx - _Nx/(_Mx*_Mx)*(log(_Nx + _Mx*vx)-log(_Nx));
		}
		else
		{
			result = vx/_Mx;
		}
	}
	else
	{
		result = vx*vx/_Nx*0.5f;
	}

	return result;
}

void GC_Vehicle::SetPlayer(const SafePtr<GC_Player> &player)
{
	new GC_IndicatorBar("indicator_health", this, &_health, &_health_max, LOCATION_TOP);
	_player = player;

	// time step fixed will be called by player
	SetEvents(0);
}

void GC_Vehicle::Serialize(SaveFile &f)
{
	GC_VehicleBase::Serialize(f);
	f.Serialize(_stateReal);
	f.Serialize(_statePredicted);
	f.Serialize(_player);
	f.Serialize(_weapon);
	f.Serialize(_visual);
}

void GC_Vehicle::Kill()
{
	SAFE_KILL(_visual);

	if( _weapon )
	{
		_weapon->SetRespawn(true);
		_weapon->Detach();
	}

	_player = NULL;

	GC_VehicleBase::Kill();
}

const vec2d& GC_Vehicle::GetPosPredicted() const
{
	return _visual->GetPos();
}

void GC_Vehicle::OnPickup(GC_Pickup *pickup, bool attached)
{
	GC_VehicleBase::OnPickup(pickup, attached);
	if( GC_Weapon *w = dynamic_cast<GC_Weapon *>(pickup) )
	{
		if( attached )
		{
			if( _weapon )
			{
				_weapon->Disappear(); // this will detach weapon and call OnPickup(attached=false)
			}

			assert(!_weapon);
			_weapon = WrapRawPtr(w);

			//
			// update class
			//

			VehicleClass vc;

			lua_State *L = g_env.L;
			lua_pushcfunction(L, luaT_ConvertVehicleClass); // function to call
			lua_getglobal(L, "getvclass");
			lua_pushstring(L, GetOwner()->GetClass().c_str());  // cls arg
			lua_pushstring(L, g_level->GetTypeName(_weapon->GetType()));  // weap arg
			if( lua_pcall(L, 2, 1, 0) )
			{
				// print error message
				GetConsole().WriteLine(1, lua_tostring(L, -1));
				lua_pop(L, 1);
				return;
			}

			lua_pushlightuserdata(L, &vc);
			if( lua_pcall(L, 2, 0, 0) )
			{
				// print error message
				GetConsole().WriteLine(1, lua_tostring(L, -1));
				lua_pop(L, 1);
				return;
			}

			SetClass(vc);
			_visual->SetClass(vc);
		}
		else
		{
			assert(_weapon);
//			Unsubscribe( GetRawPtr(_weapon) );
			_weapon = NULL;
			ResetClass();
		}
	}
}

void GC_Vehicle::SetState(const VehicleState &vs)
{
	_stateReal = vs;
}

void GC_Vehicle::SetPredictedState(const VehicleState &vs)
{
	_statePredicted = vs;
}

void GC_Vehicle::SetSkin(const string_t &skin)
{
	string_t tmp = "skin/";
	tmp += skin;
	_visual->SetTexture(tmp.c_str());
}

void GC_Vehicle::ResetClass()
{
	lua_State *L = g_env.L;
	lua_pushcfunction(L, luaT_ConvertVehicleClass); // function to call

	lua_getglobal(L, "getvclass");
	lua_pushstring(L, GetOwner()->GetClass().c_str()); // cls arg
	if( lua_pcall(L, 1, 1, 0) )  // call getvclass(clsname)
	{
		// print error message
		GetConsole().WriteLine(1, lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	VehicleClass vc;
	lua_pushlightuserdata(L, &vc);
	if( lua_pcall(L, 2, 0, 0) )
	{
		// print error message
		GetConsole().WriteLine(1, lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	SetClass(vc);
	if( _visual )
		_visual->SetClass(vc);
}

bool GC_Vehicle::TakeDamage(float damage, const vec2d &hit, GC_Player *from)
{
	assert(!IsKilled());

	DamageDesc dd;
	dd.damage = damage;
	dd.hit    = hit;
	dd.from   = from;

	PulseNotify(NOTIFY_DAMAGE_FILTER, &dd);
	if( 0 == dd.damage )
	{
		return false;
	}
	SetHealthCur(GetHealth() - dd.damage);
	{
		SafePtr<GC_Object> refHolder(this); // this may be killed during script execution
		TDFV(from ? from->GetVehicle() : NULL);
		if( IsKilled() )
		{
			// TODO: score
			return true;
		}
	}

	FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
	{
		if( !pCamera->GetPlayer() ) continue;
		if( this == pCamera->GetPlayer()->GetVehicle() )
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

		if( from )
		{
			if( from == GetOwner() )
			{
				// killed it self
				GetOwner()->SetScore(GetOwner()->GetScore() - 1);
				font = "font_digits_red";
				wsprintf(msg, g_lang.msg_player_x_killed_him_self.Get().c_str(), GetOwner()->GetNick().c_str());
			}
			else if( GetOwner() )
			{
				if( 0 != GetOwner()->GetTeam() &&
					from->GetTeam() == GetOwner()->GetTeam() )
				{
					// 'from' killed his friend
					from->SetScore(from->GetScore() - 1);
					font = "font_digits_red";
					wsprintf(msg, g_lang.msg_player_x_killed_his_friend_x.Get().c_str(),
						dd.from->GetNick().c_str(),
						GetOwner()->GetNick().c_str());
				}
				else
				{
					// 'from' killed his enemy
					from->SetScore(from->GetScore() + 1);
					font = "font_digits_green";
					wsprintf(msg, g_lang.msg_player_x_killed_his_enemy_x.Get().c_str(),
						from->GetNick().c_str(), GetOwner()->GetNick().c_str());
				}
			}
			else
			{
				// this tank does not have player service. score up the killer
				from->SetScore(from->GetScore() + 1);
				font = "font_digits_green";
			}

			if( from->GetVehicle() )
			{
				wsprintf(score, "%d", from->GetScore());
				new GC_Text_ToolTip(from->GetVehicle()->GetPos(), score, font);
			}
		}
		else if( GetOwner() )
		{
			wsprintf(msg, g_lang.msg_player_x_died.Get().c_str(), GetOwner()->GetNick().c_str());
			GetOwner()->SetScore(GetOwner()->GetScore() - 1);
			wsprintf(score, "%d", GetOwner()->GetScore());
			new GC_Text_ToolTip(GetPos(), score, "font_digits_red");
		}

		{
			SafePtr<GC_Object> refHolder(this);
			OnDestroy();
			Kill();
		}

		static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->WriteLine(msg);
		return true;
	}
	return false;
}

#ifndef NDEBUG
void GC_Vehicle::Draw() const
{
//	GC_VehicleBase::Draw();
	for( int i = 0; i < 4; ++i )
	{
		g_level->DbgLine(GetVertex(i), GetVertex((i+1)&3));
	}
}
#endif // NDEBUG

void GC_Vehicle::TimeStepFixed(float dt)
{
	// move...
	GC_VehicleBase::TimeStepFixed( dt );
	if( IsKilled() ) return;


	// fire...
	if( _weapon && _stateReal._bState_Fire )
	{
		_weapon->Fire();
		if( IsKilled() ) return;
	}


	ApplyState(_stateReal);


	//
	// die if out of level bounds
	//
	if( GetPos().x < 0 || GetPos().x > g_level->_sx ||
		GetPos().y < 0 || GetPos().y > g_level->_sy )
	{
		if( !TakeDamage(GetHealth(), GetPos(), GetOwner()) ) Kill();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Tank_Light)
{
	ED_ACTOR("tank", "obj_tank", 1, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, 0);
	return true;
}

GC_Tank_Light::GC_Tank_Light(float x, float y)
  : GC_Vehicle(x, y)
{
//	_MaxBackSpeed = 150;
//	_MaxForvSpeed = 200;

	_visual->SetMoveSound(SND_TankMove);
	SetSkin("red");
	SetSize(37, 37.5f);

	_inv_m  = 1 /   1.0f;
	_inv_i  = 1 / 700.0f;

	_Nx = 450;
	_Ny = 5000;
	_Nw = 28;

	_Mx = 2.0f;
	_My = 2.5f;
	_Mw = 0;

	SetMaxHP(400);

}

GC_Tank_Light::GC_Tank_Light(FromFile)
  : GC_Vehicle(FromFile())
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

//	_rotator.reset(_angle0, _rotator.getv(), 3.5f, 10.0f, 30.0f);

	SetMaxHP(GetDefaultHealth());
}
*/

void GC_Tank_Light::OnDestroy()
{
	new GC_Boom_Big(GetPos(), NULL);
	GC_Vehicle::OnDestroy();
}

// end of file
