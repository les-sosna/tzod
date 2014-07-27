// 2dSprite.cpp

#include "2dSprite.h"

#include "World.h"
#include "SaveFile.h"

#include "video/TextureManager.h"
#include "video/RenderBase.h"

#include "config/Config.h"


/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_2dSprite)
{
	return true;
}

IMPLEMENT_GRID_MEMBER(GC_2dSprite, grid_sprites)

GC_2dSprite::GC_2dSprite()
  : _direction(1, 0)
  , _texId(0)
{
	SetFlags(GC_FLAG_2DSPRITE_INGRIDSET, true);
}

GC_2dSprite::GC_2dSprite(FromFile)
  : _texId(0) // for proper handling of bad save files
{
}

GC_2dSprite::~GC_2dSprite()
{
}

void GC_2dSprite::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);

	f.Serialize(_direction);
	f.Serialize(_texId);
}

void GC_2dSprite::SetTexture(const char *name)
{
	if( NULL == name )
	{
		_texId = 0;
	}
	else
	{
		_texId = g_texman->FindSprite(name);
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file
