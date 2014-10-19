// Vehicle.cpp

#include "Vehicle.h"
#include "VehicleClasses.h"

#include "World.h"
#include "WorldEvents.h"
#include "Macros.h"

#include "GameClasses.h"
#include "Camera.h"
#include "Explosion.h"
#include "particles.h"
#include "Pickup.h"
#include "Light.h"
#include "Sound.h"
#include "player.h"
#include "turrets.h"
#include "Weapons.h"

#include "SaveFile.h"

#include "config/Language.h"
#include "core/Debug.h"

void GC_Vehicle::SetMoveSound(World &world, enumSoundTemplate s)
{
	_moveSound = new GC_Sound(world, s, GetPos());
    _moveSound->Register(world);
    _moveSound->SetMode(world, SMODE_LOOP);
}

IMPLEMENT_1LIST_MEMBER(GC_Vehicle, LIST_vehicles);

GC_Vehicle::GC_Vehicle(World &world)
  : _enginePower(0)
  , _rotatePower(0)
  , _maxRotSpeed(0)
  , _maxLinSpeed(0)
  , _trackDensity(8)
  , _trackPathL(0)
  , _trackPathR(0)
  , _time_smoke(0)
{
	memset(&_state, 0, sizeof(VehicleState));
    
	_light_ambient = new GC_Light(world, GC_Light::LIGHT_POINT);
    _light_ambient->Register(world);
	_light_ambient->SetIntensity(0.8f);
	_light_ambient->SetRadius(150);
    
	_light1 = new GC_Light(world, GC_Light::LIGHT_SPOT);
    _light1->Register(world);
	_light2 = new GC_Light(world, GC_Light::LIGHT_SPOT);
    _light2->Register(world);
    
	_light1->SetRadius(300);
	_light2->SetRadius(300);
    
	_light1->SetIntensity(0.9f);
	_light2->SetIntensity(0.9f);
    
	_light1->SetOffset(290);
	_light2->SetOffset(290);
    
	_light1->SetAspect(0.4f);
	_light2->SetAspect(0.4f);
}

GC_Vehicle::GC_Vehicle(FromFile)
{
}

GC_Vehicle::~GC_Vehicle()
{
}

void GC_Vehicle::SetClass(const VehicleClass &vc)
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

void GC_Vehicle::SetMaxHP(float hp)
{
	SetHealth(hp * GetHealth() / GetHealthMax(), hp);
}

void GC_Vehicle::Serialize(World &world, SaveFile &f)
{
	GC_RigidBodyDynamic::Serialize(world, f);

	f.Serialize(_enginePower);
	f.Serialize(_rotatePower);
	f.Serialize(_maxRotSpeed);
	f.Serialize(_maxLinSpeed);
	f.Serialize(_state);
	f.Serialize(_player);
	f.Serialize(_weapon);
	f.Serialize(_shield);
	f.Serialize(_time_smoke);
	f.Serialize(_trackDensity);
	f.Serialize(_trackPathL);
	f.Serialize(_trackPathR);
	f.Serialize(_light_ambient);
	f.Serialize(_light1);
	f.Serialize(_light2);
	f.Serialize(_moveSound);
}

void GC_Vehicle::ApplyState(World &world, const VehicleState &vs)
{
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
	
	if (CheckFlags(GC_FLAG_VEHICLE_KNOWNLIGHT) && _light1->GetActive() != _state._bLight)
	{
		for( auto ls: world.eGC_Vehicle._listeners )
			ls->OnLight(*this);
	}
	_light1->SetActive(_state._bLight);
	_light2->SetActive(_state._bLight);
	_light_ambient->SetActive(_state._bLight);
	SetFlags(GC_FLAG_VEHICLE_KNOWNLIGHT, true);
}

float GC_Vehicle::GetMaxSpeed() const
{
	return std::min((_enginePower - _Nx) / _Mx, _maxLinSpeed);
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

void GC_Vehicle::SetPlayer(World &world, GC_Player *player)
{
	_player = player;
	if( _player )
	{
		if( auto vc = GetVehicleClass(_player->GetClass().c_str()) )
			SetClass(*vc);
		SetSkin(std::string("skin/") + _player->GetSkin());
	}
}

void GC_Vehicle::Kill(World &world)
{
	SAFE_KILL(world, _moveSound);
	SAFE_KILL(world, _light_ambient);
	SAFE_KILL(world, _light1);
	SAFE_KILL(world, _light2);

	if( _weapon )
	{
		_weapon->SetRespawn(true);
		_weapon->Detach(world);
	}
	
	if (_shield)
	{
		_shield->Disappear(world);
		assert(!_shield);
	}

	_player = NULL;

	GC_RigidBodyDynamic::Kill(world);
}

void GC_Vehicle::SetWeapon(World &world, GC_Weapon *weapon)
{
	if( _weapon != weapon )
	{
		_weapon = weapon;
		if( _weapon )
		{
			_weapon->MoveTo(world, GetPos());
			_weapon->SetDirection(GetDirection());
			if( auto original = GetVehicleClass(GetOwner()->GetClass().c_str()) )
			{
				VehicleClass copy = *original;
				_weapon->AdjustVehicleClass(copy);
				SetClass(copy);
			}
		}
		else
		{
			if( auto vc = GetVehicleClass(GetOwner()->GetClass().c_str()) )
				SetClass(*vc);
		}
	}
}

void GC_Vehicle::MoveTo(World &world, const vec2d &pos)
{
	if (_weapon)
		_weapon->MoveTo(world, pos);
	if (_shield)
		_shield->MoveTo(world, pos);
	GC_Actor::MoveTo(world, pos);
}

void GC_Vehicle::SetControllerState(const VehicleState &vs)
{
	_state = vs;
}

void GC_Vehicle::SetSkin(const std::string &skin)
{
	_skinTextureName = skin;
}

void GC_Vehicle::OnDamage(World &world, DamageDesc &dd)
{
	if (_shield)
		_shield->OnOwnerDamage(world, dd);

	FOREACH( world.GetList(LIST_cameras), GC_Camera, camera )
	{
		if( camera->GetPlayer() && this == camera->GetPlayer()->GetVehicle() )
		{
			camera->Shake(GetHealth() <= 0 ? 2.0f : dd.damage / GetHealthMax());
			break;
		}
	}
}

void GC_Vehicle::OnDestroy(World &world, GC_Player *by)
{
	char msg[256] = {0};
	char score[8];
	GC_Text::Style style = GC_Text::DEFAULT;
	
	if( by )
	{
		if( by == GetOwner() )
		{
			// killed him self
			GetOwner()->SetScore(world, GetOwner()->GetScore() - 1);
			style = GC_Text::SCORE_MINUS;
			sprintf(msg, g_lang.msg_player_x_killed_him_self.Get().c_str(), GetOwner()->GetNick().c_str());
		}
		else if( GetOwner() )
		{
			if( 0 != GetOwner()->GetTeam() &&
			   by->GetTeam() == GetOwner()->GetTeam() )
			{
				// 'from' killed his friend
				by->SetScore(world, by->GetScore() - 1);
				style = GC_Text::SCORE_MINUS;
				sprintf(msg, g_lang.msg_player_x_killed_his_friend_x.Get().c_str(),
						by->GetNick().c_str(),
						GetOwner()->GetNick().c_str());
			}
			else
			{
				// 'from' killed his enemy
				by->SetScore(world, by->GetScore() + 1);
				style = GC_Text::SCORE_PLUS;
				sprintf(msg, g_lang.msg_player_x_killed_his_enemy_x.Get().c_str(),
						by->GetNick().c_str(), GetOwner()->GetNick().c_str());
			}
		}
		else
		{
			// this tank does not have player service. score up the killer
			by->SetScore(world, by->GetScore() + 1);
			style = GC_Text::SCORE_PLUS;
		}
		
		if( by->GetVehicle() )
		{
			sprintf(score, "%d", by->GetScore());
			auto text = new GC_Text_ToolTip(world, score, style);
			text->Register(world);
			text->MoveTo(world, by->GetVehicle()->GetPos());
		}
	}
	else if( GetOwner() )
	{
		sprintf(msg, g_lang.msg_player_x_died.Get().c_str(), GetOwner()->GetNick().c_str());
		GetOwner()->SetScore(world, GetOwner()->GetScore() - 1);
		sprintf(score, "%d", GetOwner()->GetScore());
		auto text = new GC_Text_ToolTip(world, score, GC_Text::SCORE_MINUS);
		text->Register(world);
		text->MoveTo(world, GetPos());
	}
	world.GameMessage(msg);
	GC_RigidBodyDynamic::OnDestroy(world, by);
}

void GC_Vehicle::TimeStep(World &world, float dt)
{
	std::vector<ObjectList*> receive;
	world.grid_pickup.OverlapPoint(receive, GetPos() / LOCATION_SIZE);
	for( auto &list: receive )
	{
		list->for_each([&](ObjectList::id_type, GC_Object *o)
		{
			auto &pickup = *static_cast<GC_Pickup *>(o);
			if (pickup.GetVisible() && !pickup.GetAttached() && (_state._bState_AllowDrop || pickup.GetAutoSwitch(*this)))
			{
				float dist2 = (GetPos() - pickup.GetPos()).sqr();
				float sumRadius = GetRadius() + pickup.GetRadius();
				if (dist2 < sumRadius*sumRadius)
				{
					pickup.Attach(world, *this);
				}
			}
		});
	}
	
	
	// spawn damage smoke
	if( GetHealth() < GetHealthMax() * 0.4f )
	{
		assert(GetHealth() > 0);
		                    //    +-{ particles per second }
		_time_smoke += dt;  //    |
		float smoke_dt = 1.0f / (60.0f * (1.0f - GetHealth() / (GetHealthMax() * 0.5f)));
		for(; _time_smoke > 0; _time_smoke -= smoke_dt)
		{
			auto p = new GC_Particle(world, SPEED_SMOKE, PARTICLE_SMOKE, 1.5f);
			p->Register(world);
			p->MoveTo(world, GetPos() + vrand(frand(24.0f)));
			p->_time = frand(1.0f);
		}
	}

	ObjPtr<GC_Vehicle> watch(this);

	// move...

	if( _moveSound )
	{
		_moveSound->MoveTo(world, GetPos());
		float v = _lv.len() / GetMaxSpeed();
		_moveSound->SetSpeed (std::min(1.0f, 0.5f + 0.5f * v));
		_moveSound->SetVolume(std::min(1.0f, 0.9f + 0.1f * v));
	}
    
    
	//
	// remember position
	//
    
	vec2d trackTmp(GetDirection().y, -GetDirection().x);
	vec2d trackL = GetPos() + trackTmp*15;
	vec2d trackR = GetPos() - trackTmp*15;
    
    
	// move
	ApplyState(world, _state);
	GC_RigidBodyDynamic::TimeStep(world, dt);
    
    
	//
	// caterpillar tracks
	//
    
    vec2d tmp(GetDirection().y, -GetDirection().x);
    vec2d trackL_new = GetPos() + tmp*15;
    vec2d trackR_new = GetPos() - tmp*15;
    
    vec2d e = trackL_new - trackL;
    float len = e.len();
    e /= len;
    while( _trackPathL < len )
    {
        GC_Particle *p = new GC_ParticleDecal(world, vec2d(0,0), PARTICLE_CATTRACK, 12, e);
        p->Register(world);
        p->MoveTo(world, trackL + e * _trackPathL);
        p->SetFade(true);
        _trackPathL += _trackDensity;
    }
    _trackPathL -= len;
    
    e   = trackR_new - trackR;
    len = e.len();
    e  /= len;
    while( _trackPathR < len )
    {
        GC_Particle *p = new GC_ParticleDecal(world, vec2d(0,0), PARTICLE_CATTRACK, 12, e);
        p->Register(world);
        p->MoveTo(world, trackR + e * _trackPathR);
        p->SetFade(true);
        _trackPathR += _trackDensity;
    }
    _trackPathR -= len;
	
	// update light position
	static const vec2d delta1(0.6f);
	static const vec2d delta2(-0.6f);
	_light1->MoveTo(world, GetPos() + Vec2dAddDirection(GetDirection(), delta1) * 20 );
	_light1->SetLightDirection(GetDirection());
	_light2->MoveTo(world, GetPos() + Vec2dAddDirection(GetDirection(), delta2) * 20 );
	_light2->SetLightDirection(GetDirection());
	_light_ambient->MoveTo(world, GetPos());
	
    
    if( !watch ) return;


	// fire...
	if( _weapon )
	{
		_weapon->Fire(world, _state._bState_Fire);
		if( !watch ) return;
	}

	//
	// die if out of level bounds
	//
	if( GetPos().x < 0 || GetPos().x > world._sx ||
		GetPos().y < 0 || GetPos().y > world._sy )
	{
	//	if( !TakeDamage(world, GetHealth(), GetPos(), GetOwner()) )
        {
            Kill(world);
        }
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Tank_Light)
{
	ED_ACTOR("tank", "obj_tank", 1, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, 0);
	return true;
}

GC_Tank_Light::GC_Tank_Light(World &world)
  : GC_Vehicle(world)
{
//	_MaxBackSpeed = 150;
//	_MaxForvSpeed = 200;

	SetMoveSound(world, SND_TankMove);
	SetSkin("skin/red");
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

void GC_Tank_Light::OnDestroy(World &world, GC_Player *by)
{
	MakeExplosionBig(world, GetPos(), nullptr);
	GC_Vehicle::OnDestroy(world, by);
}

// end of file
