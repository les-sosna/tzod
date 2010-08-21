// 2dSprite.cpp

#include "stdafx.h"

#include "2dSprite.h"
#include "Level.h"

#include "fs/SaveFile.h"

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

GC_2dSprite::GC_2dSprite()
  : GC_Actor()
  , _zOrderPrefered(Z_NONE)
  , _zOrderCurrent(Z_NONE)
  , _direction(1, 0)
  , _color(0xffffffff)
  , _texId(0)
  , _frame(0)
{
	SetFlags(GC_FLAG_2DSPRITE_VISIBLE|GC_FLAG_2DSPRITE_INGRIDSET, true);
}

GC_2dSprite::GC_2dSprite(FromFile)
  : GC_Actor(FromFile())
  , _zOrderCurrent(Z_NONE) // for proper handling of bad save files
  , _texId(0) // for proper handling of bad save files
{
}

GC_2dSprite::~GC_2dSprite()
{
	SetZ_current(Z_NONE);
}

void GC_2dSprite::Serialize(SaveFile &f)
{
	GC_Actor::Serialize(f);

	f.Serialize(_color);
	f.Serialize(_frame);
	f.Serialize(_direction);
	f.Serialize(_texId);
	f.Serialize(_zOrderCurrent);
	f.Serialize(_zOrderPrefered);

	assert(g_texman->IsValidTexture(_texId));

	if( f.loading() )
		UpdateCurrentZ();
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

void GC_2dSprite::SetZ_current(enumZOrder z)
{
	assert(0 <= z && Z_COUNT > z || Z_NONE == z);
	if( _zOrderCurrent == z ) return;


	//
	// clear previous z
	//

	if( Z_NONE != _zOrderCurrent )
	{
		if( CheckFlags(GC_FLAG_2DSPRITE_INGRIDSET) )
			RemoveContext(&g_level->z_grids[_zOrderCurrent]);
		else
			g_level->z_globals[_zOrderCurrent].erase(_globalZPos);
	}

	//
	// set new z
	//

	_zOrderCurrent = z;
	UpdateCurrentZ();
}

void GC_2dSprite::SetGridSet(bool bGridSet)
{
	enumZOrder current = _zOrderCurrent;
	SetZ_current(Z_NONE);
	SetFlags(GC_FLAG_2DSPRITE_INGRIDSET, bGridSet);
	SetZ_current(current);
}

void GC_2dSprite::UpdateCurrentZ()
{
	if( Z_NONE == _zOrderCurrent )
		return;

	if( CheckFlags(GC_FLAG_2DSPRITE_INGRIDSET) )
	{
		AddContext( &g_level->z_grids[_zOrderCurrent] );
	}
	else
	{
		g_level->z_globals[_zOrderCurrent].push_front(this);
		_globalZPos = g_level->z_globals[_zOrderCurrent].begin();
	}
}

void GC_2dSprite::SetZ(enumZOrder z)
{
	assert(z < Z_COUNT || z == Z_NONE);
	if( _zOrderPrefered == z )
	{
		return;
	}
	_zOrderPrefered = z;
	if( CheckFlags(GC_FLAG_2DSPRITE_VISIBLE) )
	{
		SetZ_current(z);
	}
}

enumZOrder GC_2dSprite::GetZ() const
{
	return _zOrderPrefered;
}

void GC_2dSprite::SetVisible(bool bShow)
{
	assert(!bShow || !IsKilled()); // we should now show killed objects
	if( CheckFlags(GC_FLAG_2DSPRITE_VISIBLE) == bShow )
	{
		return;
	}
	SetFlags(GC_FLAG_2DSPRITE_VISIBLE, bShow);
	SetZ_current(bShow ? _zOrderPrefered : Z_NONE);
}

void GC_2dSprite::SetFrame(int frame)
{
	assert(0 <= frame && frame < GetFrameCount());
	_frame = frame;
}

void GC_2dSprite::Draw() const
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
