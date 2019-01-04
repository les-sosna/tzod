#include "TypeReg.h"
#include "inc/gc/Decal.h"
#include "inc/gc/SaveFile.h"
#include "inc/gc/World.h"

IMPLEMENT_SELF_REGISTRATION(GC_Decal)
{
	return true;
}

GC_Decal::GC_Decal(vec2d pos, DecalType dtype, float lifeTime, float age)
	: GC_MovingObject(pos)
	, _timeCreated(-age)
	, _timeLife(lifeTime)
	, _rotationSpeed(0)
	, _dtype(dtype)
{
	assert(_timeLife > age);
}

GC_Decal::GC_Decal(FromFile)
	: GC_MovingObject(FromFile())
{
}

void GC_Decal::Init(World &world)
{
	GC_MovingObject::Init(world);
	// _timeCreated may contain initial age which is stored as negative value
	world.Timeout(*this, _timeLife + _timeCreated);
	_timeCreated += world.GetTime(); // resulting creation time may be in the past
}

void GC_Decal::Resume(World &world)
{
	Kill(world);
}

void GC_Decal::Serialize(World &world, SaveFile &f)
{
	GC_MovingObject::Serialize(world, f);
	f.Serialize(_sizeOverride);
	f.Serialize(_timeCreated);
	f.Serialize(_timeLife);
	f.Serialize(_rotationSpeed);
	f.Serialize(_dtype);
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_DecalExplosion)
{
	return true;
}

IMPLEMENT_SELF_REGISTRATION(GC_DecalGauss)
{
	return true;
}
