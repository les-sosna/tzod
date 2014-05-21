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
  , _zOrderCurrent(Z_NONE)
  , _zOrderPrefered(Z_NONE)
{
	SetFlags(GC_FLAG_2DSPRITE_VISIBLE|GC_FLAG_2DSPRITE_INGRIDSET, true);
}

GC_2dSprite::GC_2dSprite(FromFile)
  : GC_Actor(FromFile())
  , _texId(0) // for proper handling of bad save files
  , _zOrderCurrent(Z_NONE) // for proper handling of bad save files
{
}

GC_2dSprite::~GC_2dSprite()
{
}

void GC_2dSprite::Kill(World &world)
{
    SetZ_current(world, Z_NONE);
    GC_Actor::Kill(world);
}

void GC_2dSprite::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);

	f.Serialize(_color);
	f.Serialize(_frame);
	f.Serialize(_direction);
	f.Serialize(_texId);
	f.Serialize(_zOrderCurrent);
	f.Serialize(_zOrderPrefered);

	assert(g_texman->IsValidTexture(_texId));

	if( f.loading() )
		UpdateCurrentZ(world);
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

void GC_2dSprite::SetZ_current(World &world, enumZOrder z)
{
	assert(0 <= z && Z_COUNT > z || Z_NONE == z);
	if( _zOrderCurrent == z ) return;


	//
	// clear previous z
	//

	if( Z_NONE != _zOrderCurrent )
	{
		if( CheckFlags(GC_FLAG_2DSPRITE_INGRIDSET) )
			RemoveContext(&world.z_grids[_zOrderCurrent]);
		else
			world.z_globals[_zOrderCurrent].erase(_globalZPos);
	}

	//
	// set new z
	//

	_zOrderCurrent = z;
	UpdateCurrentZ(world);
}

void GC_2dSprite::SetGridSet(World &world, bool bGridSet)
{
	enumZOrder current = _zOrderCurrent;
	SetZ_current(world, Z_NONE);
	SetFlags(GC_FLAG_2DSPRITE_INGRIDSET, bGridSet);
	SetZ_current(world, current);
}

void GC_2dSprite::UpdateCurrentZ(World &world)
{
	if( Z_NONE == _zOrderCurrent )
		return;

	if( CheckFlags(GC_FLAG_2DSPRITE_INGRIDSET) )
	{
		AddContext( &world.z_grids[_zOrderCurrent] );
	}
	else
	{
		_globalZPos = world.z_globals[_zOrderCurrent].insert(this);
	}
}

void GC_2dSprite::SetZ(World &world, enumZOrder z)
{
	assert(z < Z_COUNT || z == Z_NONE);
	if( _zOrderPrefered == z )
	{
		return;
	}
	_zOrderPrefered = z;
	if( CheckFlags(GC_FLAG_2DSPRITE_VISIBLE) )
	{
		SetZ_current(world, z);
	}
}

enumZOrder GC_2dSprite::GetZ() const
{
	return _zOrderPrefered;
}

void GC_2dSprite::SetVisible(World &world, bool bShow)
{
	if( CheckFlags(GC_FLAG_2DSPRITE_VISIBLE) == bShow )
	{
		return;
	}
	SetFlags(GC_FLAG_2DSPRITE_VISIBLE, bShow);
	SetZ_current(world, bShow ? _zOrderPrefered : Z_NONE);
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
