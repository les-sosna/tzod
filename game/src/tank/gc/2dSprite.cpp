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
	width  = (float) lt.frame_width;
	height = (float) lt.frame_height;
	color  = lt.color;
	pivot.Set(lt.pivot_x, lt.pivot_y);
}


/////////////////////////////////////////////////////////////
//class GC_2dSprite - базовый класс для графических объектов

IMPLEMENT_SELF_REGISTRATION(GC_2dSprite)
{
	return true;
}

GC_2dSprite::GC_2dSprite()
  : GC_Actor()
{
	_zOrderPrefered = Z_NONE;
	_zOrderCurrent  = Z_NONE;
	SetFlags(GC_FLAG_2DSPRITE_VISIBLE | GC_FLAG_2DSPRITE_INGRIDSET);

	_width  = 0;
	_height = 0;
	_rotation = 0;
	_color.dwColor = 0xffffffff;

	_texId = 0;
	_frame = -1;

	SetPivot(0,0);
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
	f.Serialize(_frameRect);
	f.Serialize(_height);
	f.Serialize(_frame);
	f.Serialize(_pivot);
	f.Serialize(_rotation);
	f.Serialize(_texId);
	f.Serialize(_width);
	f.Serialize(_zOrderCurrent);
	f.Serialize(_zOrderPrefered);

	_ASSERT(g_texman->IsValidTexture(_texId));

	if( f.loading() )
		UpdateCurrentZ();
}

int GC_2dSprite::GetFrameCount() const
{
	return g_texman->Get(_texId).xframes * g_texman->Get(_texId).yframes;
}

void GC_2dSprite::MoveTo(const vec2d &pos)
{
	GC_Actor::MoveTo(pos);
}

void GC_2dSprite::SetTexture(const char *name)
{
	_frame = -1;
	if( NULL == name )
	{
		_texId   = 0;
		_width   = 0;
		_height  = 0;
		_color   = 0x00000000;
		_pivot.Set(0, 0);
	}
	else
	{
		_texId = g_texman->FindTexture(name);
		const LogicalTexture &lt = g_texman->Get(_texId);

		_color.r  = lt.color.r;
		_color.g  = lt.color.g;
		_color.b  = lt.color.b;

		_width  = lt.frame_width;
		_height = lt.frame_height;

		SetFrame(0);
		_pivot.Set(lt.pivot_x, lt.pivot_y);
	}
}

void GC_2dSprite::SetTexture(const TextureCache &tc)
{
	_frame = -1;

	_color.r = tc.color.r;
	_color.g = tc.color.g;
	_color.b = tc.color.b;

	_texId  = tc.texture;
	_width  = tc.width;
	_height = tc.height;
	_pivot  = tc.pivot;

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
	if( _frame == frame ) return;

	const LogicalTexture &lt = g_texman->Get(_texId);

	_frameRect.left   = (lt.left + lt.frame_width * (float) (frame % lt.xframes)) * lt.pixel_width;
	_frameRect.right  = _frameRect.left + lt.frame_width * lt.pixel_width;
	_frameRect.top    = (lt.top + lt.frame_height * (float) (frame / lt.xframes)) * lt.pixel_height;
	_frameRect.bottom = _frameRect.top + lt.frame_height * lt.pixel_height;

	_frame = frame;
}

void GC_2dSprite::UpdateTexture()
{
	if( -1 == _frame ) return; // FIXME

	const LogicalTexture &lt = g_texman->Get(_texId);

	_width  = (float) lt.frame_width;
	_height = (float) lt.frame_height;

	if( _frame >= GetFrameCount() )
		_frame  = lt.xframes * lt.yframes - 1;

	_frameRect.left   = (lt.left + lt.frame_width * (float) (_frame % lt.xframes)) * lt.pixel_width;
	_frameRect.right  = _frameRect.left + lt.frame_width * lt.pixel_width;
	_frameRect.top    = (lt.top + lt.frame_height * (float) (_frame / lt.xframes)) * lt.pixel_height;
	_frameRect.bottom = _frameRect.top + lt.frame_height * lt.pixel_height;

	if( CheckFlags(GC_FLAG_2DSPRITE_CENTERPIVOT) )
		CenterPivot();
	else
		SetPivot(lt.pivot_x, lt.pivot_y);
}

void GC_2dSprite::ModifyFrameBounds(const LPFRECT lpFrame)
{
	_ASSERT(-1 != _texId);
	_frame  = -1;

	FRECT newFrame;
	newFrame.left   = _frameRect.left + (_frameRect.right - _frameRect.left) * lpFrame->left;
	newFrame.right  = _frameRect.left + (_frameRect.right - _frameRect.left) * lpFrame->right;
	newFrame.top    = _frameRect.top  + (_frameRect.bottom - _frameRect.top) * lpFrame->top;
	newFrame.bottom = _frameRect.top  + (_frameRect.bottom - _frameRect.top) * lpFrame->bottom;

	_frameRect = newFrame;
}

void GC_2dSprite::SetPivot(const vec2d &pivot)
{
	_pivot = pivot;
}

void GC_2dSprite::SetPivot(float x, float y)
{
	_pivot.Set(x, y);
}

void GC_2dSprite::CenterPivot()
{
	SetPivot(0.5f * GetSpriteWidth(), 0.5f * GetSpriteHeight());
	SetFlags(GC_FLAG_2DSPRITE_CENTERPIVOT);
}

void GC_2dSprite::Kill()
{
	Show(false);
	GC_Actor::Kill();
}

void GC_2dSprite::Draw()
{
	g_texman->Bind(_texId);

	float px, py;
	int i = 1;

	vec2d pos = GetPosPredicted();

	SpriteColor tmp_color;

	if( !g_conf->sv_nightmode->Get() && CheckFlags(GC_FLAG_2DSPRITE_DROPSHADOW) )
	{
		tmp_color.dwColor = 0x00000000;
		tmp_color.a = _color.a >> 2;
		px = pos.x + 4;
		py = pos.y + 4;
		i  = 0;
	}

	for( ; i < 2; ++i )
	{
		MyVertex *v = g_render->DrawQuad();

		if( i )
		{
			px = pos.x;
			py = pos.y;
			tmp_color = _color;
		}

		if( 0 == _rotation )
		{
			v[0].color = tmp_color;
			v[0].u = _frameRect.left;
			v[0].v = _frameRect.top;
			v[0].x = px - _pivot.x;
			v[0].y = py - _pivot.y;

			v[1].color = tmp_color;
			v[1].u = _frameRect.right;
			v[1].v = _frameRect.top;
			v[1].x = px + _width - _pivot.x;
			v[1].y = py - _pivot.y;

			v[2].color = tmp_color;
			v[2].u = _frameRect.right;
			v[2].v = _frameRect.bottom;
			v[2].x = px + _width - _pivot.x;
			v[2].y = py + _height - _pivot.y;

			v[3].color = tmp_color;
			v[3].u = _frameRect.left;
			v[3].v = _frameRect.bottom;
			v[3].x = px - _pivot.x;
			v[3].y = py + _height - _pivot.y;
		}
		else
		{
			float c = cosf(_rotation), s = sinf(_rotation);

			v[0].color = tmp_color;
			v[0].u = _frameRect.left;
			v[0].v = _frameRect.top;
			v[0].x = px - _pivot.x * c + _pivot.y * s;
			v[0].y = py - _pivot.x * s - _pivot.y * c;

			v[1].color = tmp_color;
			v[1].u = _frameRect.right;
			v[1].v = _frameRect.top;
			v[1].x = px + (_width - _pivot.x) * c + _pivot.y * s;
			v[1].y = py + (_width - _pivot.x) * s - _pivot.y * c;

			v[2].color = tmp_color;
			v[2].u = _frameRect.right;
			v[2].v = _frameRect.bottom;
			v[2].x = px + (_width - _pivot.x) * c - (_height - _pivot.y) * s;
			v[2].y = py + (_width - _pivot.x) * s + (_height - _pivot.y) * c;

			v[3].color = tmp_color;
			v[3].u = _frameRect.left;
			v[3].v = _frameRect.bottom;
			v[3].x = px - _pivot.x * c - (_height - _pivot.y) * s;
			v[3].y = py - _pivot.x * s + (_height - _pivot.y) * c;
		}
	}

//	_FpsCounter::Inst()->OneMoreSprite();
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
