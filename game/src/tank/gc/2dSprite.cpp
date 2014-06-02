// 2dSprite.cpp

#include "2dSprite.h"

#include "World.h"
#include "SaveFile.h"

#include "video/TextureManager.h"
#include "video/RenderBase.h"

#include "config/Config.h"

///////////////////////////////////////////////////////////////////////////////

TextureCache::TextureCache(const char *name)
{
	assert(NULL != name);
	texture = g_texman->FindSprite(name);
	const LogicalTexture &lt = g_texman->Get(texture);
	width  = lt.pxFrameWidth;
	height = lt.pxFrameHeight;
}

/////////////////////////////////////////////////////////////
//class GC_2dSprite

IMPLEMENT_SELF_REGISTRATION(GC_2dSprite)
{
	return true;
}

GC_2dSprite::GC_2dSprite(World &world)
  : GC_Actor(world)
  , _direction(1, 0)
  , _color(0xffffffff)
  , _texId(0)
  , _frame(0)
  , _zOrder(Z_NONE)
{
	SetFlags(GC_FLAG_2DSPRITE_VISIBLE|GC_FLAG_2DSPRITE_INGRIDSET, true);
    AddContext(&world.grid_sprites);
}

GC_2dSprite::GC_2dSprite(FromFile)
  : GC_Actor(FromFile())
  , _texId(0) // for proper handling of bad save files
  , _zOrder(Z_NONE) // for proper handling of bad save files
{
}

GC_2dSprite::~GC_2dSprite()
{
}

void GC_2dSprite::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);

	f.Serialize(_color);
	f.Serialize(_frame);
	f.Serialize(_direction);
	f.Serialize(_texId);
	f.Serialize(_zOrder);

	assert(g_texman->IsValidTexture(_texId));
}

void GC_2dSprite::SetTexture(const char *name)
{
	_frame = -1;
	if( NULL == name )
	{
		_texId   = 0;
	}
	else
	{
		_texId = g_texman->FindSprite(name);
		SetFrame(0);
	}
}

void GC_2dSprite::SetTexture(const TextureCache &tc)
{
	_texId  = tc.texture;
	SetFrame(0);
}

void GC_2dSprite::SetZ(enumZOrder z)
{
	assert(z < Z_COUNT || z == Z_NONE);
	_zOrder = z;
}

enumZOrder GC_2dSprite::GetZ() const
{
	return _zOrder;
}

void GC_2dSprite::SetVisible(World &world, bool bShow)
{
	SetFlags(GC_FLAG_2DSPRITE_VISIBLE, bShow);
}

void GC_2dSprite::SetFrame(int frame)
{
	assert(0 <= frame && frame < GetFrameCount());
	_frame = frame;
}

void GC_2dSprite::Draw(bool editorMode) const
{
	vec2d pos = GetPos();

	if( !g_conf.sv_nightmode.Get() && CheckFlags(GC_FLAG_2DSPRITE_DROPSHADOW) )
	{
		SpriteColor tmp_color = 0x00000000;
		tmp_color.a = _color.a >> 2;
		g_texman->DrawSprite(_texId, _frame, tmp_color, pos.x + 4, pos.y + 4, _direction);
	}

	g_texman->DrawSprite(_texId, _frame, _color, pos.x, pos.y, _direction);
}

///////////////////////////////////////////////////////////////////////////////
// end of file
