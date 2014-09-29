#include "SoundHarness.h"
#include "SoundRender.h"
#include "gc/crate.h"
#include "gc/RigidBody.h"
#include "gc/World.h"

SoundHarness::SoundHarness(World &world)
	: _world(world)
	, _soundRender(new SoundRender())
{
	_world.eGC_RigidBodyStatic.AddListener(*this);
}

SoundHarness::~SoundHarness()
{
	_world.eGC_RigidBodyStatic.RemoveListener(*this);
}

void SoundHarness::Step()
{
	_soundRender->Step();
}

void SoundHarness::OnDestroy(GC_RigidBodyStatic &obj)
{
	ObjectType type = obj.GetType();
	if (GC_Crate::GetTypeStatic() == type)
	{
		_soundRender->PlayOnce(SND_WallDestroy, obj.GetPos());
	}
}
