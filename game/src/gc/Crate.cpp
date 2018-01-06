#include "inc/gc/Crate.h"
#include "inc/gc/Particles.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include "TypeReg.h"

IMPLEMENT_SELF_REGISTRATION(GC_Crate)
{
	ED_ACTOR("crate", "obj_crate", 0, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE/2, 0);
	return true;
}

GC_Crate::GC_Crate(vec2d pos)
  : GC_RigidBodyDynamic(pos)
{
	SetSize(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE);

	_Mx = _My = _Mw = 0;
	_Nx = _Ny = 400;
	_Nw = 40;

	_inv_m = 5.0f;
	_inv_i = _inv_m * 3.0f / (GetHalfWidth()*GetHalfWidth() + GetHalfLength()*GetHalfLength());

	SetHealth(30, 30);
}

GC_Crate::GC_Crate(FromFile)
  : GC_RigidBodyDynamic(FromFile())
{
}

GC_Crate::~GC_Crate()
{
}

void GC_Crate::OnDestroy(World &world, const DamageDesc &dd)
{
	for( int n = 0; n < 5; ++n )
	{
		world.New<GC_BrickFragment>(GetPos() + vrand(GetRadius()), vec2d{ frand(100.0f) - 50.f, -frand(100.0f) });
	}

	GC_RigidBodyDynamic::OnDestroy(world, dd);
}
