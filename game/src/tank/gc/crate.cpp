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

GC_Crate::GC_Crate(World &world, float x, float y)
  : GC_RigidBodyDynamic(world)
{
	MoveTo(world, vec2d(x, y));
	SetZ(world, Z_WALLS);
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
		(new GC_Brick_Fragment_01(world, GetPos() + vrand(GetRadius()),
			vec2d(frand(100.0f) - 50, -frand(100.0f))
			))->SetShadow(true);
	}

	GC_RigidBodyDynamic::OnDestroy(world);
}


// end of file
