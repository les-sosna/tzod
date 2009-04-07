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
	_ASSERT(NULL != name);
	texture = g_texman->FindTexture(name);
	const LogicalTexture &lt = g_texman->Get(texture);
	width  = lt.pxFrameWidth;
	height = lt.pxFrameHeight;
	color  = lt.color;
}


/////////////////////////////////////////////////////////////
//class GC_2dSprite - базовый класс для графических объектов

IMPLEMENT_SELF_REGISTRATION(GC_2dSprite)
{
	return true;
}

GC_2dSprite::GC_2dSprite()
  : GC_Actor()
  , _zOrderPrefered(Z_NONE)
  , _zOrderCurrent(Z_NONE)
  , _rotation(0)
  , _color(0xffffffff)
  , _texId(0)
  , _frame(0)
{
	SetFlags(GC_FLAG_2DSPRITE_VISIBLE | GC_FLAG_2DSPRITE_INGRIDSET);
}

GC_2dSprite::GC_2dSprite(FromFile)
  : GC_Actor(FromFile())
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
	f.Serialize(_rotation);
	f.Serialize(_texId);
	f.Serialize(_zOrderCurrent);
	f.Serialize(_zOrderPrefered);

	_ASSERT(g_texman->IsValidTexture(_texId));

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
		_texId = g_texman->FindTexture(name);
		SetFrame(0);
	}
}

void GC_2dSprite::SetTexture(const TextureCache &tc)
{
	_texId  = tc.texture;
	SetFrame(0);
}

// изменение текущего значения z-order
void GC_2dSprite::SetZ_current(enumZOrder z)
{
	_ASSERT(0 <= z && Z_COUNT > z || Z_NONE == z);
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
	bGridSet?SetFlags(GC_FLAG_2DSPRITE_INGRIDSET):ClearFlags(GC_FLAG_2DSPRITE_INGRIDSET);
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
	_ASSERT(z < Z_COUNT || z == Z_NONE);
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

void GC_2dSprite::Show(bool bShow)
{
	_ASSERT(!bShow || !IsKilled()); // нельзя показывать убитые объекты
	if( CheckFlags(GC_FLAG_2DSPRITE_VISIBLE) == bShow )
	{
		return;
	}
	bShow?SetFlags(GC_FLAG_2DSPRITE_VISIBLE):ClearFlags(GC_FLAG_2DSPRITE_VISIBLE);
	SetZ_current(bShow ? _zOrderPrefered : Z_NONE);
}

void GC_2dSprite::SetFrame(int frame)
{
	_ASSERT(0 <= frame && frame < GetFrameCount());
	_frame = frame;
}

void GC_2dSprite::Kill()
{
	Show(false);
	GC_Actor::Kill();
}

void GC_2dSprite::Draw()
{
	vec2d pos = GetPosPredicted();

	if( !g_conf->sv_nightmode->Get() && CheckFlags(GC_FLAG_2DSPRITE_DROPSHADOW) )
	{
		SpriteColor tmp_color = 0x00000000;
		tmp_color.a = _color.a >> 2;
		g_texman->DrawSprite(_texId, _frame, tmp_color, pos.x + 4, pos.y + 4, _rotation);
	}

	g_texman->DrawSprite(_texId, _frame, _color, pos.x, pos.y, _rotation);
}


//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_UserSprite)
{
	return true;
}

GC_UserSprite::GC_UserSprite()
  : GC_2dSprite()
{
}

GC_UserSprite::GC_UserSprite(FromFile)
  : GC_2dSprite(FromFile())
{
}

///////////////////////////////////////////////////////////////////////////////
// end of file
