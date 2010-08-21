// crate.cpp

#include "stdafx.h"
#include "crate.h"
#include "Level.h"
#include "Sound.h"

#include "particles.h"
#include "config/Config.h"
#include "functions.h"


IMPLEMENT_SELF_REGISTRATION(GC_Crate)
{
	ED_ACTOR("crate", "obj_crate", 0, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, 0);
	return true;
}

GC_Crate::GC_Crate(float x, float y)
{
	MoveTo(vec2d(x, y));
	SetZ(Z_WALLS);
	SetTexture("crate01");
	AlignToTexture();

	_Mx = _My = _Mw = 0;
	_Nx = _Ny = 400;
	_Nw = 40;

	_inv_m = 5.0f;
	_inv_i = _inv_m*12.0f / (GetSpriteWidth()*GetSpriteWidth()+GetSpriteHeight()*GetSpriteHeight());

	SetHealth(30, 30);
	g_level->_field.ProcessObject(this, true);
}

GC_Crate::GC_Crate(FromFile)
  : GC_RigidBodyDynamic(FromFile())
{
}

GC_Crate::~GC_Crate()
{
}

void GC_Crate::OnDestroy()
{
	g_level->_field.ProcessObject(this, false);
	PLAY(SND_WallDestroy, GetPos());

	if( g_conf.g_particles.Get() )
	{
		for( int n = 0; n < 5; ++n )
		{
			(new GC_Brick_Fragment_01( GetPos() + vrand(GetRadius()),
				vec2d(frand(100.0f) - 50, -frand(100.0f))
				))->SetShadow(true);
		}
	}

	GC_RigidBodyDynamic::OnDestroy();
	g_level->_field.ProcessObject(this, true);
}


// end of file
