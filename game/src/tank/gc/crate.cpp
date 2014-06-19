// crate.cpp

#include "crate.h"
#include "particles.h"
#include "Sound.h"

#include "config/Config.h"


IMPLEMENT_SELF_REGISTRATION(GC_Crate)
{
	ED_ACTOR("crate", "obj_crate", 0, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, 0);
	return true;
}

GC_Crate::GC_Crate(World &world)
  : GC_RigidBodyDynamic(world)
{
	SetTexture("crate01");
	AlignToTexture();

	_Mx = _My = _Mw = 0;
	_Nx = _Ny = 400;
	_Nw = 40;

	_inv_m = 5.0f;
	_inv_i = _inv_m*12.0f / (GetSpriteWidth()*GetSpriteWidth()+GetSpriteHeight()*GetSpriteHeight());

	SetHealth(30, 30);
}

GC_Crate::GC_Crate(FromFile)
  : GC_RigidBodyDynamic(FromFile())
{
}

GC_Crate::~GC_Crate()
{
}

void GC_Crate::OnDestroy(World &world)
{
	PLAY(SND_WallDestroy, GetPos());

	for( int n = 0; n < 5; ++n )
	{
		auto p = new GC_Brick_Fragment_01(world, vec2d(frand(100.0f) - 50, -frand(100.0f)));
        p->Register(world);
        p->MoveTo(world, GetPos() + vrand(GetRadius()));
	}

	GC_RigidBodyDynamic::OnDestroy(world);
}


// end of file
