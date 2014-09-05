// 2dSprite.cpp

#include "2dSprite.h"
#include "globals.h"
#include "World.h"
#include "SaveFile.h"

#include <video/TextureManager.h>
#include <video/RenderBase.h>

IMPLEMENT_GRID_MEMBER(GC_2dSprite, grid_sprites)

GC_2dSprite::GC_2dSprite()
  : _texId(0)
{
}

GC_2dSprite::~GC_2dSprite()
{
}

float GC_2dSprite::GetSpriteWidth() const
{
	return g_texman->Get(_texId).pxFrameWidth;
}

float GC_2dSprite::GetSpriteHeight() const
{
	return g_texman->Get(_texId).pxFrameHeight;
}

void GC_2dSprite::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);
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
