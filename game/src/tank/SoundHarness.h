#pragma once
#include "gc/WorldEvents.h"
#include <memory>

class SoundRender;
class World;

class SoundHarness
	: ObjectListener<GC_RigidBodyStatic>
{
public:
	SoundHarness(World &world);
	~SoundHarness();
	
	void Step();
	
private:
	World &_world;
	std::unique_ptr<SoundRender> _soundRender;
	
	// ObjectListener<GC_RigidBodyStatic>
	virtual void OnDestroy(GC_RigidBodyStatic &obj) override;
	virtual void OnDamage(GC_RigidBodyStatic &obj, GC_Actor *from) override {}

	// ObjectListener<GC_Object>
//	virtual void OnCreate(GC_Object &obj) override {}
//	virtual void OnKill(GC_Object &obj) override {}
};
