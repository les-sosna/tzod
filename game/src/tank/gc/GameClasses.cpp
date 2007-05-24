//GameClasses.cpp

#include "stdafx.h"

#include "core/Debug.h"
#include "core/JobManager.h"
#include "core/Console.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"

#include "ui/GuiManager.h"

#include "network/TankClient.h"

#include "video/RenderBase.h"

#include "config/Config.h"

#include "functions.h"
#include "Macros.h"
#include "Options.h"
#include "Level.h"

#include "ai.h"

#include "GameClasses.h"
#include "Editor.h"
#include "Vehicle.h"
#include "Pickup.h"
#include "Sound.h"
#include "Player.h"
#include "particles.h"

/////////////////////////////////////////////////////////////
// класс GC_Winamp - модуль управления проигрывателем Winamp.

#include "Frontend.h"


IMPLEMENT_SELF_REGISTRATION(GC_Winamp)
{
	return true;
}

GC_Winamp::GC_Winamp() : GC_Object()
{
	FindWinamp();

	_time_last = timeGetTime();
	_time = 0;

	_last_b1	= 0;
	_last_b2	= 0;
	_last_b3	= 0;
	_last_b4	= 0;
	_last_b5	= 0;
	_last_up	= 0;
	_last_down	= 0;
	_last_ffw	= 0;
	_last_rew	= 0;
	////////////////////////////
	SetEvents(GC_FLAG_OBJECT_EVENTS_ENDFRAME);
}

void GC_Winamp::FindWinamp()
{
	_hwnd_winamp = FindWindow("Winamp v1.x",NULL);
}

void GC_Winamp::EndFrame()
{
	if( !_hwnd_winamp ) return;

	int time_current = timeGetTime();
	int local_dt = time_current - _time_last;
	_time_last = time_current;
	_time += local_dt;

	if( g_env.envInputs.keys[g_options.wkWinampKeys.keyVolumeUp] )
	{
		for (; _time > 0; _time -= 20)
			PostMessage(_hwnd_winamp, WM_COMMAND, WINAMP_VOLUMEUP, 0);
	}
	else if( g_env.envInputs.keys[g_options.wkWinampKeys.keyVolumeDown] )
	{
		for (; _time > 0; _time -= 20)
			PostMessage(_hwnd_winamp, WM_COMMAND, WINAMP_VOLUMEDOWN, 0);
	}
	else
	{
		_time = 0;
	}

	SendCommand(_hwnd_winamp, WINAMP_BUTTON1, &_last_b1,  &g_env.envInputs.keys[g_options.wkWinampKeys.keyButton1]);
	SendCommand(_hwnd_winamp, WINAMP_BUTTON2, &_last_b2,  &g_env.envInputs.keys[g_options.wkWinampKeys.keyButton2]);
	SendCommand(_hwnd_winamp, WINAMP_BUTTON3, &_last_b3,  &g_env.envInputs.keys[g_options.wkWinampKeys.keyButton3]);
	SendCommand(_hwnd_winamp, WINAMP_BUTTON4, &_last_b4,  &g_env.envInputs.keys[g_options.wkWinampKeys.keyButton4]);
	SendCommand(_hwnd_winamp, WINAMP_BUTTON5, &_last_b5,  &g_env.envInputs.keys[g_options.wkWinampKeys.keyButton5]);
	SendCommand(_hwnd_winamp, WINAMP_FFWD5S,  &_last_ffw, &g_env.envInputs.keys[g_options.wkWinampKeys.keyFfwd5s] );
	SendCommand(_hwnd_winamp, WINAMP_REW5S,   &_last_rew, &g_env.envInputs.keys[g_options.wkWinampKeys.keyRew5s]  );
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Camera)
{
	return true;
}

GC_Camera::GC_Camera(GC_Player *pPlayer)
  : GC_Object(), _memberOf(g_level->cameras, this), _rotator(_angle_current)
{
	_player = pPlayer;
	_rotator.reset(0.0f, 0.0f, 2.0f, 0.5f, 0.5f);
	if( _player )
	{
		MoveTo( vec2d(g_level->_sx / 2, g_level->_sy / 2) );
		if( _player->_vehicle )
		{
//			_angle_current =  -_player->_vehicle->_dir + PI/2;
			MoveTo( _player->_vehicle->_pos );
		}
		SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
		_player->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Camera::OnDetach);
		_player->Subscribe(NOTIFY_PLAYER_SETCONTROLLER, this, (NOTIFYPROC) &GC_Camera::OnDetach);

		_active = !g_level->_modeEditor;
	}
	else
	{
		MoveTo( vec2d(0, 0) );
		SetEvents(GC_FLAG_OBJECT_EVENTS_ENDFRAME);
		_active = g_level->_modeEditor;
	}
	//---------------------------------------
	_target     = _pos;
	_time_shake = 0;
	_time_seed  = frand(1000);
	_zoom       = 1.0f;
	//---------------------------------------
	_dwTimeX = _dwTimeY = GetTickCount();
	_dt      = 50.0f;
	//---------------------------------------
	UpdateLayout();
}

GC_Camera::GC_Camera(FromFile)
  : GC_Object(), _memberOf(g_level->cameras, this), _rotator(_angle_current)
{
}

void GC_Camera::TimeStepFloat(float dt)
{
	float mu = 3;

//	_rotator.process_dt(dt);
	if( _player->_vehicle )
	{
//		_rotator.rotate_to(-_player->_vehicle->_dir - PI/2);

		mu += _player->_vehicle->_lv.Length() / 100;

		int dx = (int) __max(0, ((float) WIDTH(_viewport) / _zoom  - g_level->_sx) / 2);
		int dy = (int) __max(0, ((float) HEIGHT(_viewport) / _zoom - g_level->_sy) / 2);

		vec2d r = _player->_vehicle->_pos + _player->_vehicle->_lv / mu;

		if( _player->_vehicle->_weapon )
		{
			r += vec2d(_player->_vehicle->_player->_vehicle->_angle +
				_player->_vehicle->_weapon->_angle)*130.0f;
		}
		else
		{
			r += _player->_vehicle->_direction * 130.0f;
		}

		_target.x = r.x + (float) dx;
		_target.y = r.y + (float) dy;

		_target.x = __max(_target.x, (float) WIDTH(_viewport) / _zoom * 0.5f + dx);
		_target.x = __min(_target.x, g_level->_sx - (float) WIDTH(_viewport) / _zoom * 0.5f + dx);
		_target.y = __max(_target.y, (float) HEIGHT(_viewport) / _zoom * 0.5f + dy);
		_target.y = __min(_target.y, g_level->_sy - (float) HEIGHT(_viewport) / _zoom * 0.5f + dy);
	}

	if (_time_shake > 0)
	{
		_time_shake -= dt;
		if (_time_shake < 0) _time_shake = 0;
	}

	MoveTo(_target + (_pos - _target) * expf(-dt * mu));
}

void GC_Camera::Select()
{
	g_render->SetViewport(&_viewport);

	if( _player )
	{
		vec2d shake(0, 0);
		if( _time_shake > 0 )
		{
			shake.Set(
				cosf((_time_shake + _time_seed)*70.71068f),
				sinf((_time_shake + _time_seed)*86.60254f) );
			shake *= CELL_SIZE * _time_shake * 0.1f;
		}

		g_env.camera_x = int(floorf((_pos.x + shake.x - (float)  WIDTH(_viewport) / _zoom * 0.5f) * _zoom) / _zoom);
		g_env.camera_y = int(floorf((_pos.y + shake.y - (float) HEIGHT(_viewport) / _zoom * 0.5f) * _zoom) / _zoom);
	}
	else
	{
		g_env.camera_x = int(_pos.x);
		g_env.camera_y = int(_pos.y);
	}

	g_render->Camera((float) g_env.camera_x,
	                 (float) g_env.camera_y, 
					 _zoom, 
					 _angle_current);
}

void GC_Camera::Activate(bool bActivate)
{
	_ASSERT(!IsKilled());
	_active = bActivate;
}

void GC_Camera::EndFrame()
{
	if( !IsActive() ) return;

	static char  LastIn   = 0, LastOut = 0;
	static float levels[] = { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f };
	static int   level    = 4;

//	if( !LastIn && g_env.envInputs.keys[DIK_PGUP] )
//		level = __min(level+1, sizeof(levels) / sizeof(float)-1);
	LastIn = g_env.envInputs.keys[DIK_PGUP];

//	if( !LastOut && g_env.envInputs.keys[DIK_PGDN] )
//		level = __max(level-1, 0);
	LastOut = g_env.envInputs.keys[DIK_PGDN];

	_zoom = levels[level];


	int   x         = g_env.envInputs.mouse_x;
	int   y         = g_env.envInputs.mouse_y;
	bool  bMove     = false;
	DWORD dwCurTime = GetTickCount();
	DWORD dt        = DWORD(_dt);
	//---------------------------------------
	if( 0 == x || g_env.envInputs.keys[DIK_LEFTARROW] )
	{
		bMove = true;
		while (dwCurTime - _dwTimeX > dt)
		{
			_pos.x -= CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else
	if( g_render->getXsize() - 1 == x || g_env.envInputs.keys[DIK_RIGHTARROW] )
	{
		bMove = true;
		while (dwCurTime - _dwTimeX > dt)
		{
			_pos.x += CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else
		_dwTimeX = GetTickCount();
	//---------------------------------------
	if( 0 == y || g_env.envInputs.keys[DIK_UPARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			_pos.y -= CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else
	if( g_render->getYsize()-1 == y || g_env.envInputs.keys[DIK_DOWNARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			_pos.y += CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else
		_dwTimeY = GetTickCount();
	//---------------------------------------
	if (bMove)
		_dt = __max(10.0f, 1.0f / (1.0f / _dt + 0.001f));
	else
		_dt = 50.0f;
	//------------------------------------------------------
	int dx = __max(0, (int) ((float)  WIDTH(_viewport) / _zoom - g_level->_sx) / 2);
	int dy = __max(0, (int) ((float) HEIGHT(_viewport) / _zoom - g_level->_sy) / 2);
	_pos.x = (float) __max(int(_pos.x), dx);
	_pos.x = (float) __min(int(_pos.x), int(g_level->_sx - (float) WIDTH(_viewport) / _zoom) + dx);
	_pos.y = (float) __max(int(_pos.y), dy);
	_pos.y = (float) __min(int(_pos.y), int(g_level->_sy - (float) HEIGHT(_viewport) / _zoom) + dy);
}

void GC_Camera::SwitchEditor()
{
	UpdateLayout();
}

void GC_Camera::UpdateLayout()
{
	GC_Camera *tmp = NULL;
	size_t active_count = 0;

	ENUM_BEGIN(cameras, GC_Camera, pCamera)
	{
		if( !pCamera->IsKilled() )
		{
			tmp = pCamera;

			if( tmp->_player )
				tmp->Activate(!g_level->_modeEditor);
			else
				tmp->Activate(g_level->_modeEditor);

			if( tmp->IsActive() ) 
				++active_count;
		}
	} ENUM_END();

	if( tmp && 0 == active_count )
	{
		tmp->Activate(true);
		++active_count;
	}

	RECT viewports[MAX_HUMANS];

	if( g_render->getXsize() >= (int)g_level->_sx &&
		g_render->getYsize() >= (int)g_level->_sy )
	{
		SetRect(&viewports[0],
			(g_render->getXsize() - (int) g_level->_sx) / 2,
			(g_render->getYsize() - (int) g_level->_sy) / 2,
			(g_render->getXsize() + (int) g_level->_sx) / 2,
			(g_render->getYsize() + (int) g_level->_sy) / 2
		);
		ENUM_BEGIN(cameras, GC_Camera, pCamera)	{
			pCamera->_viewport = viewports[0];
		} ENUM_END();
	}
	else
	{
		if( 0 == active_count ) return;

		int w = g_render->getXsize();
		int h = g_render->getYsize();

		switch( active_count )
		{
		case 1:
			SetRect(&viewports[0], 0, 0, w, h );
			break;
		case 2:
			SetRect(&viewports[0], 0, 0, w/2 - 1, h );
			SetRect(&viewports[1], w/2 + 1, 0, w, h );
			break;
		case 3:
			SetRect(&viewports[0], 0, 0, w/2 - 1, h/2 - 1 );
			SetRect(&viewports[1], w/2 + 1, 0, w, h/2 - 1 );
			SetRect(&viewports[2], w/4, h/2 + 1, w*3/4, h );
			break;
		case 4:
			SetRect(&viewports[0], 0, 0, w/2 - 1, h/2 - 1 );
			SetRect(&viewports[1], w/2 + 1, 0, w, h/2 - 1 );
			SetRect(&viewports[2], 0, h/2 + 1, w/2 - 1, h );
			SetRect(&viewports[3], w/2 + 1, h/2 + 1, w, h );
			break;
		default:
			_ASSERT(false);
		}

		size_t count = 0;
		float  zoom  = active_count > 2 ? 0.5f : 1.0f;
		ENUM_BEGIN(cameras, GC_Camera, pCamera)
		{
			if( !pCamera->IsActive() ) continue;
			pCamera->_viewport = viewports[count++];
			pCamera->_zoom     = zoom;
		} ENUM_END();
	}
}

bool GC_Camera::GetWorldMousePos(vec2d &pos)
{
	POINT ptinscr = { g_env.envInputs.mouse_x, g_env.envInputs.mouse_y };

	ENUM_BEGIN(cameras, GC_Camera, pCamera)
	{
		if( !pCamera->IsActive() ) continue;
		if( PtInRect(&pCamera->_viewport, ptinscr) )
		{
			pCamera->Select();
			pos.x = (float) (ptinscr.x - pCamera->_viewport.left) / pCamera->_zoom + (float) g_env.camera_x;
            pos.y = (float) (ptinscr.y - pCamera->_viewport.top) / pCamera->_zoom + (float) g_env.camera_y;
			return true;
		}
	} ENUM_END();
	return false;
}

void GC_Camera::Shake(float level)
{
	_ASSERT(_player);
	if( 0 == _time_shake )
		_time_seed = frand(1000.0f);
	_time_shake = __min(_time_shake + 0.5f * level, PLAYER_RESPAWNTIME / 2);
}

void GC_Camera::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_Object::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_angle_current);
	f.Serialize(_active);
	f.Serialize(_dt);
	f.Serialize(_dwTimeX);
	f.Serialize(_dwTimeY);
	f.Serialize(_target);
	f.Serialize(_time_seed);
	f.Serialize(_time_shake);
	f.Serialize(_zoom);
	f.Serialize(_player);
	/////////////////////////////////////
	_rotator.Serialize(f);
	if( f.loading() ) UpdateLayout();
}

void GC_Camera::Kill()
{
	_player = NULL;
	Activate(false);
	//-------------------
	GC_Object::Kill();
	//-------------------
	UpdateLayout();
}

void GC_Camera::OnDetach(GC_Object *sender, void *param)
{
	Kill();
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Background)
{
	return true;
}

GC_Background::GC_Background() : GC_2dSprite()
{
	MoveTo(vec2d(0, 0));
	_drawGrid = g_level->_modeEditor && g_conf.ed_drawgrid->Get();
}

void GC_Background::Draw()
{
	{
		SetTexture("background");
		FRECT rt  = {0};
		rt.right  = g_level->_sx / GetSpriteWidth();
		rt.bottom = g_level->_sy / GetSpriteHeight();
        ModifyFrameBounds(&rt);
		Resize(g_level->_sx, g_level->_sy);
		SetPivot(0,0);
		GC_2dSprite::Draw();
	}

	if( _drawGrid && g_level->_modeEditor )
	{
		SetTexture("grid");
		FRECT rt  = {0};
		rt.right  = g_level->_sx / GetSpriteWidth();
		rt.bottom = g_level->_sy / GetSpriteHeight();
        ModifyFrameBounds(&rt);
		Resize(g_level->_sx, g_level->_sy);
		SetPivot(0,0);
		GC_2dSprite::Draw();
	}
}

void GC_Background::EnableGrid(bool bEnable)
{
	_drawGrid = bEnable;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Wood)
{
	ED_LAND( "wood", "Ландшафт: лес                  ",  2 );
	return true;
}

GC_Wood::GC_Wood(float xPos, float yPos) : GC_2dSprite()
{
	AddContext( &g_level->grid_wood );

	SetZ(Z_WOOD);

	SetTexture("wood");
	CenterPivot();
	MoveTo( vec2d(xPos, yPos) );
	SetFrame(4);

	_tile = 0;
	UpdateTile(true);
}

GC_Wood::GC_Wood(FromFile) : GC_2dSprite(FromFile())
{
}

void GC_Wood::UpdateTile(bool flag)
{
	static char tile1[9] = {5, 6, 7, 4,-1, 0, 3, 2, 1};
	static char tile2[9] = {1, 2, 3, 0,-1, 4, 7, 6, 5};
	///////////////////////////////////////////////////
	FRECT frect;
	GetGlobalRect(frect);
	frect.left   = frect.left / LOCATION_SIZE - 0.5f;
	frect.top    = frect.top  / LOCATION_SIZE - 0.5f;
	frect.right  = frect.right  / LOCATION_SIZE + 0.5f;
	frect.bottom = frect.bottom / LOCATION_SIZE + 0.5f;

	PtrList<OBJECT_LIST> receive;

	g_level->grid_wood.OverlapRect(receive, frect);
	///////////////////////////////////////////////////
	PtrList<OBJECT_LIST>::iterator rit = receive.begin();
	for( ; rit != receive.end(); ++rit )
	{
		OBJECT_LIST::iterator it = (*rit)->begin();
		for( ; it != (*rit)->end(); ++it )
		{
			GC_Wood *object = (GC_Wood *) (*it);
			if (this == object) continue;

			vec2d dx = (_pos - object->_pos) / CELL_SIZE;
			if (dx.Square() < 2.5f)
			{
				int x = int(dx.x + 1.5f);
				int y = int(dx.y + 1.5f);

				object->SetTile(tile1[x + y * 3], flag);
				SetTile(tile2[x + y * 3], flag);
			}
		}
	}
}

void GC_Wood::Kill()
{
    UpdateTile(false);
	GC_2dSprite::Kill();
}

void GC_Wood::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_tile);
	/////////////////////////////////////
	if( !IsKilled() && f.loading() )
		AddContext(&g_level->grid_wood);
}

void GC_Wood::Draw()
{
	static float dx[8]   = {-32,-32,  0, 32, 32, 32,  0,-32 };
	static float dy[8]   = {  0,-32,-32,-32,  0, 32, 32, 32 };
	static int frames[8] = {  5,  8,  7,  6,  3,  0,  1,  2 };

	if( !g_level->_modeEditor )
	{
		SetOpacity1i(255);

		for (char i = 0; i < 8; ++i)
		{
			if ( 0 == (_tile & (1 << i)) )
			{
				SetPivot(dx[i] + GetSpriteWidth() * 0.5f, dy[i] + GetSpriteHeight() * 0.5f);
				SetFrame(frames[i]);
				GC_2dSprite::Draw();
			}
		}

		CenterPivot();
		SetFrame(4);
	}
	else
	{
		SetOpacity1i(128);
	}

	GC_2dSprite::Draw();
}

void GC_Wood::SetTile(char nTile, bool value)
{
	_ASSERT(0 <= nTile && nTile < 8);

	if( value )
		_tile |=  (1 << nTile);
	else
		_tile &= ~(1 << nTile);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Line)
{
	return true;
}

GC_Line::GC_Line(const vec2d &begin, const vec2d &end, const char *texture)
{
	_frame = 0;
	_phase = 0;
	_begin = begin;
	_end   = end;

	SetGridSet(false);

	SetRotation((end-begin).Angle());
	SetTexture(texture);
	_sprite_width = GetSpriteWidth();

	GC_UserSprite::MoveTo((begin+end) * 0.5f);
}

void GC_Line::SetPhase(float f)
{
	float len = (_end-_begin).Length();

	SetFrame(_frame);

	_phase = fmodf(f, 1);
	FRECT rt;
	rt.left = _phase;
	rt.right = rt.left + len / _sprite_width;
	rt.top = 0;
	rt.bottom = 1;

	ModifyFrameBounds(&rt);
	Resize(len, GetSpriteHeight());
	CenterPivot();
}

void GC_Line::SetLineView(int index)
{
	_frame = index;
	SetPhase(_phase);
}

void GC_Line::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_UserSprite::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_begin);
	f.Serialize(_end);
	f.Serialize(_phase);
	f.Serialize(_sprite_width);
	/////////////////////////////////////
}

void GC_Line::Draw()
{
	for( int i = 0; i < 2; ++i )
	{
		SetPhase(-_phase + 0.5f);
		GC_UserSprite::Draw();
	}
}

void GC_Line::MoveTo(const vec2d &begin, const vec2d &end)
{
	_begin = begin;
	_end   = end;
	SetRotation((end-begin).Angle());
	SetPhase(_phase);
}

//////////////////////////////////////////////////////////////////////////////////////////////

/*
IMPLEMENT_SELF_REGISTRATION(GC_Rectangle)
{
	return true;
}
*/

GC_Rectangle::GC_Rectangle(const vec2d &pos, const vec2d &size, const char *texture)
	:GC_Line(pos-size*0.5f, pos+size*0.5f, texture)
{
	_center_pos = pos;
	_size       = size;
}

/*
GC_Rectangle::GC_Rectangle(FromFile) : GC_Line(FromFile())
{
}
*/

void GC_Rectangle::SetSize(const vec2d &size)
{
	_size = size;
}

void GC_Rectangle::Draw()
{
	GC_Line::MoveTo(vec2d(_center_pos.x, _center_pos.y - _size.y * 0.5f));
	GC_Line::MoveTo(_pos-_size*0.5f, vec2d(_pos.x+_size.x*0.5f, _pos.y-_size.y*0.5f));
	GC_Line::Draw();

	GC_Line::MoveTo(vec2d(_center_pos.x, _center_pos.y + _size.y * 0.5f));
	GC_Line::MoveTo(vec2d(_pos.x-_size.x*0.5f, _pos.y+_size.y*0.5f), _pos+_size*0.5f);
	GC_Line::Draw();

	GC_Line::MoveTo(vec2d(_center_pos.x + _size.x * 0.5f, _center_pos.y));
	GC_Line::MoveTo(vec2d(_pos.x-_size.x*0.5f, _pos.y+_size.y*0.5f), _pos-_size*0.5f);
	GC_Line::Draw();

	GC_Line::MoveTo(vec2d(_center_pos.x - _size.x * 0.5f, _center_pos.y));
	GC_Line::MoveTo(vec2d(_pos.x-_size.x*0.5f, _pos.y+_size.y*0.5f), _pos-_size*0.5f);
	GC_Line::Draw();

	GC_Line::MoveTo(_center_pos);
}

void GC_Rectangle::MoveTo(const vec2d &center_pos)
{
	_center_pos = center_pos;
	GC_Line::MoveTo(center_pos);
}

void GC_Rectangle::Adjust(GC_2dSprite *object)
{
	FRECT frect;
	object->GetGlobalRect(frect);
	SetSize(vec2d(WIDTH(frect), HEIGHT(frect)));
	MoveTo(vec2d(frect.right+frect.left, frect.top+frect.bottom) * 0.5f);
}

///////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Explosion)
{
	return true;
}

GC_Explosion::GC_Explosion(GC_RigidBodyStatic *pProprietor) : GC_2dSprite()
{
	SetZ(Z_EXPLODE);

	_time = 0;
	_time_life = 0.5f;
	_time_boom = 0;

	_Damage = 1;
	_DamRadius = 32;

	SetRotation(frand(PI2));

	_boomOK = false;

	_proprietor = pProprietor;
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);

	_light = new GC_Light(GC_Light::LIGHT_POINT);
}

GC_Explosion::GC_Explosion(FromFile) : GC_2dSprite(FromFile())
{
}

GC_Explosion::~GC_Explosion()
{
	_ASSERT(!_proprietor);
}

void GC_Explosion::Kill()
{
	_proprietor = NULL;
	SAFE_KILL(_light);
	GC_2dSprite::Kill();
}

void GC_Explosion::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_2dSprite::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_boomOK);
	f.Serialize(_Damage);
	f.Serialize(_DamRadius);
	f.Serialize(_time);
	f.Serialize(_time_boom);
	f.Serialize(_time_life);
	/////////////////////////////////////
	f.Serialize(_light);
	f.Serialize(_proprietor);
}

float GC_Explosion::CheckDamage(FIELD_TYPE &field, float dst_x, float dst_y, float max_distance)
{
	int x0 = int(_pos.x / CELL_SIZE);
	int y0 = int(_pos.y / CELL_SIZE);
	int x1 = int(dst_x   / CELL_SIZE);
	int y1 = int(dst_y   / CELL_SIZE);

	std::deque<_tagFieldNode *> open;
	open.push_front(&field[coord(x0, y0)]);
	open.front()->checked  = true;
	open.front()->distance = 0;
	open.front()->x = x0;
	open.front()->y = y0;


	while( !open.empty() )
	{
		_tagFieldNode *node = open.back();
		open.pop_back();

		if( x1 == node->x && y1 == node->y )
		{
			// путь найден.
			return node->GetRealDistance();
		}



		//
		// проверка соседей
		//
		//  4 | 0  | 6
		// ---+----+---
		//  2 |node| 3
		// ---+----+---
		//  7 | 1  | 5		//   0  1  2  3  4  5  6  7
		static int per_x[8] = {  0, 0,-1, 1,-1, 1, 1,-1 };
		static int per_y[8] = { -1, 1, 0, 0,-1, 1,-1, 1 };
		static int dist [8] = { 12,12,12,12,17,17,17,17 };	// стоимость пути

		static int check_diag[] = { 0,2,  1,3,  3,0,  2,1 };


		for( int i = 0; i < 8; ++i )
		{
			if (i > 3)
			if( !field[coord(node->x + per_x[check_diag[(i-4)*2  ]], node->y + per_y[check_diag[(i-4)*2  ]])].open &&
				!field[coord(node->x + per_x[check_diag[(i-4)*2+1]], node->y + per_y[check_diag[(i-4)*2+1]])].open )
			{
				continue;
			}

			coord coord_(node->x + per_x[i], node->y + per_y[i]);
			_tagFieldNode *next = &field[coord_];
			next->x = coord_.x;
			next->y = coord_.y;

			if( next->open )
			{
				if( !next->checked )
				{
					next->checked  = true;
					next->parent   = node;
					next->distance = node->distance + dist[i];
					if( next->GetRealDistance() <= max_distance)
						open.push_front(next);
				}
				else if( next->distance > node->distance + dist[i] )
				{
					next->parent   = node;
					next->distance = node->distance + dist[i];
					if( next->GetRealDistance() <= max_distance )
						open.push_front(next);
				}
			}
		}
	}

	return -1;
}

void GC_Explosion::Boom(float radius, float damage)
{
	ENUM_BEGIN(cameras, GC_Camera, pCamera)
	{
		if( !pCamera->_player ) continue;
		if( pCamera->_player->_vehicle )
		{
			float level = 0.5f * (radius - (_pos -
				pCamera->_player->_vehicle->_pos).Length() * 0.3f) / radius;
			//--------------------------
			if( level > 0 )
				pCamera->Shake(level);
		}
	} ENUM_END();

	///////////////////////////////////////////////////////////

	FIELD_TYPE field;

	_tagFieldNode node;
	node.open = false;

	//
	// получение списка локаций, накрываемых взрывом
	//
	std::vector<OBJECT_LIST*> receive;
	g_level->grid_rigid_s.OverlapCircle(receive,
		_pos.x / LOCATION_SIZE, _pos.y / LOCATION_SIZE, radius / LOCATION_SIZE);

	//
	// подготовка поля для трассировки
	//
	std::vector<OBJECT_LIST*>::iterator it = receive.begin();
	for (; it != receive.end(); ++it)
	{
		OBJECT_LIST::iterator cdit = (*it)->begin();
		while(cdit != (*it)->end())
		{
			GC_RigidBodyStatic *pDamObject = (GC_RigidBodyStatic *) (*cdit);
			++cdit;

			if (pDamObject->IsKilled())	continue;
			if( GC_Wall_Concrete::this_type == pDamObject->GetType() )
			{
				node.x = int(pDamObject->_pos.x / CELL_SIZE);
				node.y = int(pDamObject->_pos.y / CELL_SIZE);
				field[coord(node.x, node.y)] = node;
			}
		}
	}

	//
	// трассировка до ближайших объектов
	//

	bool bNeedClean = false;
	for (it = receive.begin(); it != receive.end(); ++it)
	{
		OBJECT_LIST::iterator cdit = (*it)->begin();
		while(cdit != (*it)->end())
		{
			GC_RigidBodyStatic *pDamObject = (GC_RigidBodyStatic *) (*cdit);
			++cdit;

			if (pDamObject->IsKilled())	continue;


			FRECT frtObjColRect;
			pDamObject->GetAABB(&frtObjColRect);

			vec2d dam = pDamObject->_pos;

			float d = (_pos - dam).Length();

			if( d <= radius)
			{
				GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) g_level->agTrace(
					g_level->grid_rigid_s, NULL, _pos, dam - _pos);

				if( object && object != pDamObject )
				{
					if( bNeedClean )
					{
						FIELD_TYPE::iterator it = field.begin();
						while( it != field.end() )
							(it++)->second.checked = false;
					}
					d = CheckDamage(field, dam.x, dam.y, radius);
					bNeedClean = true;
				}

				if( d >= 0 )
				{
					float dam = __max(0, damage * (1 - d / radius));
					if( dam > 0 ) pDamObject->TakeDamage( dam, _pos, GetRawPtr(_proprietor) );
				}
			}
		}
	}

	_proprietor = NULL;
	_boomOK = true;
}

void GC_Explosion::TimeStepFixed(float dt)
{
	GC_2dSprite::TimeStepFixed(dt);

	_time += dt;
	if( _time >= _time_boom && !_boomOK )
		Boom(_DamRadius, _Damage);

	if( _time >= _time_life )
	{
		if( _time >= _time_life * 1.5f )
		{
			Kill();
			return;
		}
		Show(false);
	}
	else
	{
		SetFrame(int((float)GetFrameCount() * _time / _time_life));
	}

	_light->SetIntensity(1.0f - powf(_time / (_time_life * 1.5f - _time_boom), 6));
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Boom_Standard)
{
	return true;
}

GC_Boom_Standard::GC_Boom_Standard(const vec2d &pos, GC_RigidBodyStatic *pProprietor) : GC_Explosion(pProprietor)
{
	static const TextureCache tex1("particle_1");
	static const TextureCache tex2("particle_smoke");
	static const TextureCache tex3("smallblast");

	_DamRadius = 70;
	_Damage    = 150;

	_time_life = 0.32f;
	_time_boom = 0.03f;

	SetTexture("explosion_o");
	MoveTo( pos );

	if( g_options.bParticles )
	{
		for(int n = 0; n < 28; ++n)
		{
			//ring
			float ang = frand(PI2);
			new GC_Particle(pos, vec2d(ang) * 100, tex1, frand(0.5f) + 0.1f);

			//smoke
			ang = frand(PI2);
			float d = frand(64.0f) - 32.0f;

			(new GC_Particle(_pos + vec2d(ang) * d, SPEED_SMOKE, tex2, 1.5f))
				->_time = frand(1.0f);
		}
		GC_Particle *p = new GC_Particle(_pos, vec2d(0,0), tex3, 8.0f, frand(PI2));
		p->SetZ(Z_WATER);
		p->SetFade(true);
	}

	_light->SetRadius(_DamRadius * 5);
	_light->MoveTo(_pos);

	PLAY(SND_BoomStandard, _pos);
}

GC_Boom_Standard::GC_Boom_Standard(FromFile) : GC_Explosion(FromFile())
{
}

GC_Boom_Standard::~GC_Boom_Standard()
{
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Boom_Big)
{
	return true;
}

GC_Boom_Big::GC_Boom_Big(const vec2d &pos, GC_RigidBodyStatic *pProprietor) : GC_Explosion(pProprietor)
{
	static const TextureCache tex1("particle_1");
	static const TextureCache tex2("particle_2");
	static const TextureCache tex4("particle_trace");
	static const TextureCache tex5("particle_smoke");
	static const TextureCache tex6("bigblast");

	_DamRadius = 128;
	_Damage    = 90;

	_time_life = 0.72f;
	_time_boom = 0.10f;

	SetTexture("explosion_big");
	MoveTo( pos );

	if( g_options.bParticles )
	{
		for(int n = 0; n < 80; ++n)
		{
			//ring
			for( int i = 0; i < 2; ++i )
			{
				new GC_Particle(_pos + vrand(frand(20.0f)),
					vrand((200.0f + frand(30.0f)) * 0.9f), tex1, frand(0.6f) + 0.1f);
			}

			vec2d a;

			//dust
			a = vrand(frand(40.0f));
			new GC_Particle(_pos + a, a * 2, tex2, frand(0.5f) + 0.25f);

			// sparkles
			a = vrand(frand(40.0f));
			new GC_Particle(_pos + a, a * 2, tex4, frand(0.3f) + 0.2f, a.Angle());

			//smoke
			a = vrand(frand(48.0f));
			(new GC_Particle(_pos + a, SPEED_SMOKE + a / 2.0f,
							 tex5, 1.5f))->_time = frand(1.0f);
		}

		GC_Particle *p = new GC_Particle(_pos, vec2d(0,0), tex6, 20.0f, frand(PI2));
		p->SetZ(Z_WATER);
		p->SetFade(true);
	}

	_light->SetRadius(_DamRadius * 5);
	_light->MoveTo(_pos);

	PLAY(SND_BoomBig, _pos);
}

GC_Boom_Big::GC_Boom_Big(FromFile) : GC_Explosion(FromFile())
{
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_HealthDaemon)
{
	return true;
}

GC_HealthDaemon::GC_HealthDaemon(GC_RigidBodyStatic *pVictim, GC_RigidBodyStatic *pOwner,
								 float damage, float time)
{
	_time   = time;
	_damage = damage;

	_victim = pVictim;
	_owner  = pOwner;

	_victim->Subscribe(NOTIFY_OBJECT_MOVE, this, 
		(NOTIFYPROC) &GC_HealthDaemon::OnVictimMove, false);
	_victim->Subscribe(NOTIFY_OBJECT_KILL, this, 
		(NOTIFYPROC) &GC_HealthDaemon::OnVictimKill, true);

	MoveTo(_victim->_pos);

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED /*| GC_FLAG_OBJECT_EVENTS_TS_FLOATING*/ );
}

void GC_HealthDaemon::Kill()
{
	_victim = NULL;
	_owner  = NULL;
	GC_2dSprite::Kill();
}

void GC_HealthDaemon::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_time);
	f.Serialize(_damage);
	f.Serialize(_victim);
	f.Serialize(_owner);
}

void GC_HealthDaemon::TimeStepFloat(float dt)
{
}

void GC_HealthDaemon::TimeStepFixed(float dt)
{
	_time -= dt;
	bool bKill = false;
	if( _time <= 0 )
	{
		dt += _time;
		bKill = true;
	}
	if( !_victim->TakeDamage(dt * _damage, _victim->_pos, GetRawPtr(_owner)) && bKill )
		Kill();
}

void GC_HealthDaemon::OnVictimMove(GC_Object *sender, void *param)
{
	MoveTo(sender->_pos);
}

void GC_HealthDaemon::OnVictimKill(GC_Object *sender, void *param)
{
	Kill();
}

/////////////////////////////////////////////////////////////
//class GC_Text - отображение текста

IMPLEMENT_SELF_REGISTRATION(GC_Text)
{
	return true;
}

GC_Text::GC_Text(int xPos, int yPos, const char *lpszText, enumAlignText align)
: GC_2dSprite()
{
	SetZ(Z_SCREEN);

	SetFont("font_default");
	SetAlign(align);
	if( lpszText ) SetText(lpszText);
	SetMargins(0, 0);

	MoveTo( vec2d((float)xPos, (float)yPos) );
}

void GC_Text::SetText(const char *pText)
{
	_ASSERT(NULL != pText);
	if( _text != pText )
	{
		_text = pText;
		UpdateLines();
	}
}

void GC_Text::SetFont(const char *fontname)
{
	SetTexture(fontname);
}

void GC_Text::SetAlign(enumAlignText align)
{
	_align = align;
}

void GC_Text::SetMargins(float mx, float my)
{
	_margin_x = mx;
	_margin_y = my;
}

void GC_Text::UpdateLines()
{
	_lines.clear();
	_maxline = 0;

	size_t count = 0;
	for( const char *tmp = _text.c_str(); *tmp; )
	{
		++count;
		++tmp;
		if ('\n' == *tmp || '\0' == *tmp)
		{
			if (count > _maxline) _maxline = count;
			_lines.push_back(count);
			count = 0;
			continue;
		}
	}
}

void GC_Text::Draw()
{
	static const int dx_[] = {0, 1, 2, 0, 1, 2, 0, 1, 2};
	static const int dy_[] = {0, 0, 0, 1, 1, 1, 2, 2, 2};

	float x0 = _margin_x - (float) (dx_[_align] * (GetSpriteWidth() - 1) * _maxline / 2);
	float y0 = _margin_y - (float) (dy_[_align] * (GetSpriteHeight() - 1) * _lines.size() / 2);

	size_t count = 0;
	size_t line  = 0;

	for( const char *tmp = _text.c_str(); *tmp; ++tmp )
	{
		if( '\n' == *tmp )
		{
			++line;
			count = 0;
			continue;
		}

		SetFrame((unsigned char) *tmp - 32);
		SetPivot(-x0 - (float) ((count++) * (GetSpriteWidth() - 1)),
			-y0 - (float) (line * (GetSpriteHeight() - 1)));

		GC_2dSprite::Draw();
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TextScore)
{
	return true;
}

GC_TextScore::GC_TextScore() : GC_Text(0, 0, "score")
{
	Show(false);

	_bOldLimit = false;

	_background = new GC_2dSprite();
	_background->Show(false);
	_background->SetZ(Z_SCREEN);
	_background->SetTexture("scoretbl");

	MoveTo(vec2d(
		(float) (g_render->getXsize() - _background->GetSpriteWidth()) / 2,
		(float) (g_render->getYsize() - _background->GetSpriteHeight()) / 2));

	_background->MoveTo(_pos);
	SetEvents(GC_FLAG_OBJECT_EVENTS_ENDFRAME);
}

void GC_TextScore::Kill()
{
	SAFE_KILL(_background);
	GC_Text::Kill();
}

void GC_TextScore::Refresh()
{
	//
	// перебираем всех игроков и заносим в таблицу _players[]
	//
	_players.clear();
	ENUM_BEGIN(players, GC_Player, pPlayer)
	{
		if (pPlayer->IsKilled()) continue;
		_players.push_back( PlayerDesc() );
		_players.back().score = pPlayer->_score;
		strcpy(_players.back().name, pPlayer->_name.c_str());
	} ENUM_END();

	if( _players.empty() ) return;

	//
	// сортируем игроков по колличеству фрагов
	//
	for( int i = _players.size(); --i;)
	for( int j = 0; j < i; ++j )
	{
		if( _players[j].score < _players[j+1].score )
		{
			PlayerDesc tmp = _players[j+1];
			_players[j+1]  = _players[j];
			_players[j]    = tmp;
		}
	}
}

void GC_TextScore::EndFrame()
{
	if( g_conf.sv_timelimit->GetFloat() )
	{
		if( g_level->_time >= g_conf.sv_timelimit->GetFloat() * 60 )
		{
			g_level->Pause(true);
			g_level->_limitHit = true;
		}
	}

	Show((0 != g_env.envInputs.keys[DIK_TAB] && !g_level->_modeEditor) || g_level->_limitHit);

	if( g_level->_limitHit && !_bOldLimit )
	{
		PLAY(SND_Limit, _pos);
		_bOldLimit = true;
	}

	_background->Show(IsVisible());
	if( IsVisible() ) Refresh();
}

void GC_TextScore::Draw()
{
	char text[256];
	if( g_conf.sv_timelimit->GetFloat() )
	{
		int timeleft = int(g_conf.sv_timelimit->GetFloat() * 60.0f - g_level->_time);
		if( timeleft > 0 )
		{
			if( timeleft % 60 < 10 )
				wsprintf(text, "Осталось времени %d:0%d", timeleft / 60, timeleft % 60);
			else
				wsprintf(text, "Осталось времени %d:%d", timeleft / 60, timeleft % 60);
		}
		else
			wsprintf(text, "Достигнут лимит времени");

		SetText(text);
		SetAlign(alignTextLT);
		SetMargins(SCORE_LIMITS_LEFT, SCORE_TIMELIMIT_TOP);
		GC_Text::Draw();
	}

	if( g_conf.sv_fraglimit->GetInt() )
	{
		int max_score = _players.empty() ? 0 : _players[0].score;
		for( size_t i = 0; i < _players.size(); ++i )
		{
			if( _players[i].score > max_score )
				max_score = _players[i].score;
		}
		int scoreleft = g_conf.sv_fraglimit->GetInt() - max_score;
		if (scoreleft > 0)
			wsprintf(text, "Осталось фрагов  %d", scoreleft);
		else
			wsprintf(text, "Достигнут лимит фрагов");

		SetText(text);
		SetAlign(alignTextLT);
		SetMargins(SCORE_LIMITS_LEFT, SCORE_FRAGLIMIT_TOP);
		GC_Text::Draw();
	}

	for( size_t i = 0; i < _players.size(); ++i )
	{
		if( i < MAX_PLAYERS )
		{
			wsprintf(text, "%d", i + 1);
			SetText(text);
			SetAlign(alignTextLT);
			SetMargins(SCORE_POS_NUMBER, (float) (SCORE_NAMES_TOP + (GetSpriteHeight() - 1) * i));
			GC_Text::Draw();

			SetText(_players[i].name);
			SetMargins(SCORE_POS_NAME, (float) (SCORE_NAMES_TOP + (GetSpriteHeight() - 1) * i));
			GC_Text::Draw();

			wsprintf(text, "%d", _players[i].score);
			SetText(text);
			SetAlign(alignTextRT);
			SetMargins((float) (_background->GetSpriteWidth() - SCORE_POS_SCORE),
				(float) (SCORE_NAMES_TOP + (GetSpriteHeight() - 1) * i));
			GC_Text::Draw();
		}
		else
		{
			SetAlign(alignTextLT);
			SetText("......");
			SetMargins(SCORE_POS_NAME, (float) (SCORE_NAMES_TOP + (GetSpriteHeight() - 1) * i));
			GC_Text::Draw();
			break;
		}
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Text_ToolTip)
{
	return true;
}

GC_Text_ToolTip::GC_Text_ToolTip(vec2d pos, const char *text, const char *font)
: GC_Text(int(pos.x), int(pos.y), text, alignTextCC)
{
	_time = 0;

	SetZ(Z_PARTICLE);

	SetText(text);
	SetFont(font);

	float x_min = (float) (GetSpriteWidth() / 2);
	float x_max = g_level->_sx - x_min;

	_y0 = pos.y;
	_pos.x = __min(x_max, __max(x_min, _pos.x)) - (GetSpriteWidth() / 2);
	////////////////////////////////
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
}

void GC_Text_ToolTip::TimeStepFloat(float dt)
{
	GC_Text::TimeStepFloat(dt);
	_time += dt;
	MoveTo(vec2d(_pos.x, _y0 - _time * 20.0f));
	if (_time > 1.0f) Kill();
}

/////////////////////////////////////////////////////////////
/*
GC_TextTime::GC_TextTime(int xPos, int yPos, enumAlignText align) : GC_Text(xPos, yPos, "", align)
{
	SetFont("font_default");
	SetEvents(GC_FLAG_OBJECT_EVENTS_ENDFRAME);
}

void GC_TextTime::EndFrame()
{
	Show( g_options.bShowTime && !g_level->_modeEditor );
	if( IsVisible() )
	{
		char text[16];
		int time = int(g_level->_time);

		if( time % 60 < 10 )
			wsprintf(text, "%d:0%d", time / 60, time % 60);
		else
			wsprintf(text, "%d:%d", time / 60, time % 60);

		SetText(text);
	}
}
*/
/////////////////////////////////////////////////////////////

GC_Text_MessageArea::GC_Text_MessageArea()
: GC_Text(48, g_render->getYsize() - 128, "", alignTextLB)
{
	SetFont("font_small");
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
}

void GC_Text_MessageArea::TimeStepFloat(float dt)
{
	GC_Text::TimeStepFloat(dt);
    for( size_t i = 0; i < _lines.size(); ++i )
		_lines[i].time += dt;
	while( !_lines.empty() && _lines.front().time > 5 )
		_lines.pop_front();
	string_t str;
    for( size_t i = 0; i < _lines.size(); ++i )
		str.append(_lines[i].str);
	SetText(str.c_str());
}

void GC_Text_MessageArea::message(const char *text)
{
	Line line;
	line.time = 0;
	line.str = text;
	line.str.append("\n");
	_lines.push_back(line);
	g_console->puts(line.str.c_str());
}

///////////////////////////////////////////////////////////////////////////////
// end of file
