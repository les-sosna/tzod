#include "SoundHarness.h"
#include "SoundRender.h"
#include "gc/crate.h"
#include "gc/Pickup.h"
#include "gc/RigidBody.h"
#include "gc/Vehicle.h"
#include "gc/Weapons.h"
#include "gc/World.h"

SoundHarness::SoundHarness(World &world)
	: _world(world)
	, _soundRender(new SoundRender())
{
	_world.eGC_Pickup.AddListener(*this);
	_world.eGC_RigidBodyStatic.AddListener(*this);
	_world.eGC_Vehicle.AddListener(*this);
	_world.eWorld.AddListener(*this);
}

SoundHarness::~SoundHarness()
{
	_world.eWorld.RemoveListener(*this);
	_world.eGC_Vehicle.RemoveListener(*this);
	_world.eGC_RigidBodyStatic.RemoveListener(*this);
	_world.eGC_Pickup.RemoveListener(*this);
}

void SoundHarness::Step()
{
	_soundRender->Step();
}

void SoundHarness::OnPickup(GC_Pickup &obj, GC_Actor &actor)
{
	ObjectType type = obj.GetType();
	if (GC_pu_Health::GetTypeStatic() == type)
		_soundRender->PlayOnce(SND_Pickup, obj.GetPos());
	else if (GC_pu_Shield::GetTypeStatic() == type)
		_soundRender->PlayOnce(SND_Inv, obj.GetPos());
	else if (GC_pu_Shock::GetTypeStatic() == type)
		_soundRender->PlayOnce(SND_ShockActivate, obj.GetPos());
	else if (dynamic_cast<GC_Weapon*>(&obj))
		_soundRender->PlayOnce(SND_w_Pickup, obj.GetPos());

}

void SoundHarness::OnRespawn(GC_Pickup &obj)
{
	_soundRender->PlayOnce(SND_puRespawn, obj.GetPos());
}

void SoundHarness::OnDestroy(GC_RigidBodyStatic &obj)
{
	ObjectType type = obj.GetType();
	if (GC_Crate::GetTypeStatic() == type)
		_soundRender->PlayOnce(SND_WallDestroy, obj.GetPos());
	else if (GC_Wall::GetTypeStatic() == type)
		_soundRender->PlayOnce(SND_WallDestroy, obj.GetPos());
}

void SoundHarness::OnDamage(GC_RigidBodyStatic &obj, float damage, GC_Player *from)
{
	if (GC_Wall_Concrete::GetTypeStatic() == obj.GetType())
	{
		if( damage >= DAMAGE_BULLET )
		{
			if( rand() < RAND_MAX / 128 )
				_soundRender->PlayOnce(SND_Hit1, obj.GetPos());
			else if( rand() < RAND_MAX / 128 )
				_soundRender->PlayOnce(SND_Hit3, obj.GetPos());
			else if( rand() < RAND_MAX / 128 )
				_soundRender->PlayOnce(SND_Hit5, obj.GetPos());
		}
	}
}

void SoundHarness::OnLight(GC_Vehicle &obj)
{
	_soundRender->PlayOnce(SND_LightSwitch, obj.GetPos());
}

void SoundHarness::OnGameFinished()
{
	// FIXME: play at no specific position
	_soundRender->PlayOnce(SND_Limit, vec2d(0,0));
}

