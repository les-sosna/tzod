#include "TypeReg.h"
#include "inc/gc/Vehicle.h"
#include "inc/gc/VehicleClasses.h"
#include "inc/gc/Explosion.h"
#include "inc/gc/Light.h"
#include "inc/gc/Macros.h"
#include "inc/gc/Particles.h"
#include "inc/gc/Pickup.h"
#include "inc/gc/Player.h"
#include "inc/gc/Weapons.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldEvents.h"
#include "inc/gc/SaveFile.h"


IMPLEMENT_1LIST_MEMBER(GC_RigidBodyDynamic, GC_Vehicle, LIST_vehicles);

GC_Vehicle::GC_Vehicle(vec2d pos)
  : GC_RigidBodyDynamic(pos)
  , _enginePower(0)
  , _rotatePower(0)
  , _trackPathL(0)
  , _trackPathR(0)
  , _time_smoke(0)
{
	memset(&_state, 0, sizeof(VehicleState));
}

GC_Vehicle::GC_Vehicle(FromFile)
  : GC_RigidBodyDynamic(FromFile())
{
}

GC_Vehicle::~GC_Vehicle()
{
}

void GC_Vehicle::Init(World &world)
{
	GC_RigidBodyDynamic::Init(world);

	_light_ambient = &world.New<GC_Light>(GetPos(), GC_Light::LIGHT_POINT);
	_light_ambient->SetIntensity(0.8f);
	_light_ambient->SetRadius(150);

	_light1 = &world.New<GC_Light>(GetLightPos1(), GC_Light::LIGHT_SPOT);
	_light2 = &world.New<GC_Light>(GetLightPos2(), GC_Light::LIGHT_SPOT);

	_light1->SetRadius(300);
	_light2->SetRadius(300);

	_light1->SetIntensity(0.9f);
	_light2->SetIntensity(0.9f);

	_light1->SetOffset(290);
	_light2->SetOffset(290);

	_light1->SetAspect(0.4f);
	_light2->SetAspect(0.4f);
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

	_enginePower = vc.m * vc.maxLinSpeed * (vc._Mx * vc.maxLinSpeed + vc._Nx);
	_rotatePower = vc.i * vc.maxRotSpeed * (vc._Mw * vc.maxRotSpeed + vc._Nw);

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
	f.Serialize(_state);
	f.Serialize(_player);
	f.Serialize(_weapon);
	f.Serialize(_shield);
	f.Serialize(_time_smoke);
	f.Serialize(_trackTotalPathL);
	f.Serialize(_trackTotalPathR);
	f.Serialize(_trackPathL);
	f.Serialize(_trackPathR);
	f.Serialize(_light_ambient);
	f.Serialize(_light1);
	f.Serialize(_light2);
}

void GC_Vehicle::ApplyState(World &world, const VehicleState &vs)
{
	float throttledPower = _enginePower * std::abs(vs.gas);
	if (throttledPower > 1e-5f)
	{
		float absVx = std::abs(Vec2dDot(GetDirection(), _lv));
		float maxForce = _Ny / _inv_m;
		float sign = vs.gas > 0 ? 1.f : -1.f;
		float force = throttledPower < maxForce * absVx ? throttledPower / absVx : maxForce;
		ApplyForce(GetDirection() * force * sign);
	}

	float throttledRotatePower = _rotatePower * vs.steering.len();
	if (throttledRotatePower > 1e-5f)
	{
		vec2d eventualDirection = Vec2dAddDirection(GetDirection(), Vec2dDirection(GetSpinup()));
		float maxTorque = _Nw / _inv_i * 2;
		float torqueSign = Vec2dCross(eventualDirection, vs.steering) > 0 ? 1.f : -1.f;
		float absTorque = throttledRotatePower < maxTorque * std::abs(_av) ? throttledRotatePower / std::abs(_av) : maxTorque;
		ApplyTorque(absTorque * torqueSign);
	}

	if (CheckFlags(GC_FLAG_VEHICLE_KNOWNLIGHT) && _light1->GetActive() != _state.light)
	{
		for( auto ls: world.eGC_Vehicle._listeners )
			ls->OnLight(*this);
	}
	_light1->SetActive(_state.light);
	_light2->SetActive(_state.light);
	_light_ambient->SetActive(_state.light);
	SetFlags(GC_FLAG_VEHICLE_KNOWNLIGHT, true);
}

float GC_Vehicle::GetMaxSpeed() const
{
	if (_Mx > 0)
	{
		float d = std::sqrt(_Nx * _Nx + _Mx * _enginePower * _inv_m * 4);
		return std::max(0.f, (d - _Nx) / (_Mx * 2));
	}
	else
	{
		return _enginePower * _inv_m / _Nx;
	}
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
		if( auto vc = GetVehicleClass(_player->GetClass()) )
			SetClass(*vc);
		SetSkin(std::string("skin/").append(_player->GetSkin()));
	}
}

void GC_Vehicle::Kill(World &world)
{
	SAFE_KILL(world, _light_ambient);
	SAFE_KILL(world, _light1);
	SAFE_KILL(world, _light2);

	if( _weapon )
	{
		_weapon->SetRespawn(true);
		_weapon->Detach(world);
		assert(!_weapon);
	}

	if (_shield)
	{
		_shield->Disappear(world);
		assert(!_shield);
	}

	GC_RigidBodyDynamic::Kill(world);
}

void GC_Vehicle::SetWeapon(World &world, GC_Weapon *weapon)
{
	if( _weapon != weapon )
	{
		if (GetShield() && GetShield()->GetIsDefaultItem())
			GetShield()->Disappear(world);

		_weapon = weapon;
		if( _weapon )
		{
			_weapon->MoveTo(world, GetPos());
			_weapon->SetDirection(GetDirection());
			if( auto original = GetVehicleClass(GetOwner()->GetClass()) )
			{
				VehicleClass copy = *original;
				_weapon->AdjustVehicleClass(copy);
				SetClass(copy);
			}
		}
		else
		{
			if( auto vc = GetVehicleClass(GetOwner()->GetClass()) )
				SetClass(*vc);
		}
	}
}

void GC_Vehicle::MoveTo(World &world, const vec2d &pos)
{
	if (_weapon)
		_weapon->MoveTo(world, pos);
	if (_shield)
	{
		_shield->MoveTo(world, pos);
		_shield->SetDirection(GetDirection());
	}
	GC_MovingObject::MoveTo(world, pos);
}

void GC_Vehicle::SetControllerState(const VehicleState &vs)
{
	_state = vs;
}

void GC_Vehicle::SetSkin(std::string skin)
{
	_skinTextureName = std::move(skin);
}

void GC_Vehicle::OnDamage(World &world, DamageDesc &dd)
{
	if (_shield)
		_shield->OnOwnerDamage(world, dd);
}

void GC_Vehicle::OnDestroy(World &world, const DamageDesc &dd)
{
	if (GetOwner())
		GetOwner()->OnVehicleDestroy(world);

	GC_RigidBodyDynamic::OnDestroy(world, dd);
}

void GC_Vehicle::TimeStep(World &world, float dt)
{
	// look for pickups
	std::vector<ObjectList*> receive;
	world.grid_pickup.OverlapPoint(receive, GetPos() / WORLD_LOCATION_SIZE);
	for( auto &list: receive )
	{
		list->for_each([&](ObjectList::id_type, GC_Object *o)
		{
			auto &pickup = *static_cast<GC_Pickup *>(o);
			if (pickup.GetVisible() && !pickup.GetAttached() && (_state.pickup || pickup.ShouldPickup(*this)))
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
			world.New<GC_Particle>(GetPos() + vrand(frand(24.0f)), SPEED_SMOKE, PARTICLE_SMOKE, 1.5f, frand(1.0f));
		}
	}

	ObjPtr<GC_Vehicle> watch(this);


	//
	// remember position
	//

	vec2d trackTmp{ GetDirection().y, -GetDirection().x };
	vec2d trackL = GetPos() + trackTmp*15;
	vec2d trackR = GetPos() - trackTmp*15;


	// move
	ApplyState(world, _state);
	GC_RigidBodyDynamic::TimeStep(world, dt);


	//
	// caterpillar tracks
	//

	vec2d tmp{ GetDirection().y, -GetDirection().x };
	vec2d trackL_new = GetPos() + tmp*15;
	vec2d trackR_new = GetPos() - tmp*15;

	const float trackDensity = 8;

	vec2d e = trackL_new - trackL;
	_trackTotalPathL += Vec2dDot(e, GetDirection());
	float len = e.len();
	e /= len;
	while( _trackPathL < len )
	{
		auto &p = world.New<GC_Decal>(trackL + e * _trackPathL, PARTICLE_CATTRACK, 12.0f);
		p.SetDirection(e);
		p.SetFade(true);
		_trackPathL += trackDensity;
	}
	_trackPathL -= len;

	e   = trackR_new - trackR;
	_trackTotalPathR += Vec2dDot(e, GetDirection());
	len = e.len();
	e  /= len;
	while( _trackPathR < len )
	{
		auto &p = world.New<GC_Decal>(trackR + e * _trackPathR, PARTICLE_CATTRACK, 12.0f);
		p.SetDirection(e);
		p.SetFade(true);
		_trackPathR += trackDensity;
	}
	_trackPathR -= len;

	// update light position
	_light1->MoveTo(world, GetLightPos1());
	_light1->SetLightDirection(GetDirection());
	_light2->MoveTo(world, GetLightPos2());
	_light2->SetLightDirection(GetDirection());
	_light_ambient->MoveTo(world, GetPos());


	if( !watch ) return;


	if( _weapon )
	{
		_weapon->Fire(world, _state.attack);
		if( !watch ) return;
	}

	// die if out of level bounds
	if( !PtInFRect(world.GetBounds(), GetPos()) )
	{
		DamageDesc dd;
		dd.damage = GetHealth();
		dd.hit = GetPos();
		dd.from = GetOwner();
		Destroy(world, dd);
	}
}

vec2d GC_Vehicle::GetLightPos1() const
{
	static const vec2d delta1 = Vec2dDirection(0.6f);
	return GetPos() + Vec2dAddDirection(GetDirection(), delta1) * 20;
}

vec2d GC_Vehicle::GetLightPos2() const
{
	static const vec2d delta2 = Vec2dDirection(-0.6f);
	return GetPos() + Vec2dAddDirection(GetDirection(), delta2) * 20;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Tank_Light)
{
	ED_MOVING_OBJECT("tank", "obj_tank", 1, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE/2, 0, true /*hidden*/);
	return true;
}

GC_Tank_Light::GC_Tank_Light(vec2d pos)
  : GC_Vehicle(pos)
{
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

void GC_Tank_Light::OnDestroy(World &world, const DamageDesc &dd)
{
	world.New<GC_ExplosionBig>(GetPos());
	GC_Vehicle::OnDestroy(world, dd);
}

// end of file
