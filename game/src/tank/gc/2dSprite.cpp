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

void GC_2dSprite::GetGlobalRect(FRECT &rect) const
{
	const LogicalTexture &lt = g_texman->Get(_texId);
	rect.left   = GetPos().x - lt.pxFrameWidth * lt.uvPivot.x;
	rect.top    = GetPos().y - lt.pxFrameHeight * lt.uvPivot.y;
	rect.right  = rect.left + lt.pxFrameWidth;
	rect.bottom = rect.top  + lt.pxFrameHeight;
}

void GC_2dSprite::GetLocalRect(FRECT &rect) const
{
	const LogicalTexture &lt = g_texman->Get(_texId);
	rect.left   = -lt.uvPivot.x * lt.pxFrameWidth;
	rect.top    = -lt.uvPivot.y * lt.pxFrameHeight;
	rect.right  = rect.left + lt.pxFrameWidth;
	rect.bottom = rect.top + lt.pxFrameHeight;
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
