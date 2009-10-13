// Camera.cpp

#include "stdafx.h"

#include "Camera.h"
#include "Level.h"
#include "Player.h"
#include "Vehicle.h"
#include "Weapons.h"

#include "functions.h"
#include "macros.h"

#include "video/RenderBase.h" // FIXME

#include "fs/SaveFile.h"

#include "config/Config.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Camera)
{
	return true;
}

GC_Camera::GC_Camera(SafePtr<GC_Player> &player)
  : GC_Actor()
  , _memberOf(this)
  , _rotator(_angle_current)
  , _player(player)
{
	_rotator.reset(0.0f, 0.0f,
		g_conf.g_rotcamera_m.GetFloat(),
		g_conf.g_rotcamera_a.GetFloat(),
		__max(0.001f, g_conf.g_rotcamera_s.GetFloat()));
	if( _player )
	{
		MoveTo( vec2d(g_level->_sx / 2, g_level->_sy / 2) );
		if( _player->GetVehicle() )
		{
			_angle_current =  -_player->GetVehicle()->GetVisual()->_angle + PI/2;
			MoveTo( _player->GetVehicle()->GetPosPredicted() );
		}
		SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
		_player->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Camera::OnDetach);
		_player->Subscribe(NOTIFY_PLAYER_SETCONTROLLER, this, (NOTIFYPROC) &GC_Camera::OnDetach);

		_active = !g_level->_modeEditor;
	}
	else
	{
		MoveTo( vec2d(0, 0) );
		_active = g_level->_modeEditor;
	}
	//---------------------------------------
	_target     = GetPos();
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
  : GC_Actor(FromFile())
  , _memberOf(this)
  , _rotator(_angle_current)
{
}

void GC_Camera::TimeStepFloat(float dt)
{
	float mu = 3;

	_rotator.process_dt(dt);
	if( _player->GetVehicle() )
	{
		_rotator.rotate_to(-_player->GetVehicle()->GetVisual()->_angle - PI/2);

		mu += _player->GetVehicle()->GetVisual()->_lv.len() / 100;

		int dx = (int) __max(0, ((float) WIDTH(_viewport) / _zoom  - g_level->_sx) / 2);
		int dy = (int) __max(0, ((float) HEIGHT(_viewport) / _zoom - g_level->_sy) / 2);

		vec2d r = _player->GetVehicle()->GetPosPredicted() + _player->GetVehicle()->GetVisual()->_lv / mu;

		if( _player->GetVehicle()->GetWeapon() )
		{
			r += vec2d(_player->GetVehicle()->GetVisual()->_angle +
				_player->GetVehicle()->GetWeapon()->_angleReal)*130.0f;
		}
		else
		{
			r += _player->GetVehicle()->GetVisual()->_direction * 130.0f;
		}

		_target.x = r.x + (float) dx;
		_target.y = r.y + (float) dy;

		_target.x = __max(_target.x, (float) WIDTH(_viewport) / _zoom * 0.5f + dx);
		_target.x = __min(_target.x, g_level->_sx - (float) WIDTH(_viewport) / _zoom * 0.5f + dx);
		_target.y = __max(_target.y, (float) HEIGHT(_viewport) / _zoom * 0.5f + dy);
		_target.y = __min(_target.y, g_level->_sy - (float) HEIGHT(_viewport) / _zoom * 0.5f + dy);
	}

	if( _time_shake > 0 )
	{
		_time_shake -= dt;
		if( _time_shake < 0 ) _time_shake = 0;
	}

	MoveTo(_target + (GetPos() - _target) * expf(-dt * mu));
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

		g_env.camera_x = int(floorf((GetPos().x + shake.x - (float)  WIDTH(_viewport) / _zoom * 0.5f) * _zoom) / _zoom);
		g_env.camera_y = int(floorf((GetPos().y + shake.y - (float) HEIGHT(_viewport) / _zoom * 0.5f) * _zoom) / _zoom);
	}
	else
	{
		g_env.camera_x = int(GetPos().x);
		g_env.camera_y = int(GetPos().y);
	}

	g_render->Camera((float) g_env.camera_x,
	                 (float) g_env.camera_y,
	                 _zoom,
	                 g_conf.g_rotcamera.Get() ? _angle_current : 0);
}

void GC_Camera::Activate(bool bActivate)
{
	assert(!IsKilled());
	_active = bActivate;
}

void GC_Camera::GetViewport(RECT &vp) const
{
	vp = _viewport;
}

void GC_Camera::HandleFreeMovement()
{
	if( !IsActive() || _player ) return;

	static char  lastIn   = 0, LastOut = 0;
	static float levels[] = { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f };
	static int   level    = 4;

	if( !lastIn && g_env.envInputs.keys[DIK_PGUP] )
		level = __min(level+1, sizeof(levels) / sizeof(float) - 1);
	lastIn = g_env.envInputs.keys[DIK_PGUP];

	if( !LastOut && g_env.envInputs.keys[DIK_PGDN] )
		level = __max(level-1, 0);
	LastOut = g_env.envInputs.keys[DIK_PGDN];

	_zoom = levels[level];

	vec2d pos = GetPos();

	bool  bMove     = false;
	DWORD dwCurTime = GetTickCount();
	DWORD dt        = DWORD(_dt);
	//---------------------------------------
	if( 0 == g_env.envInputs.mouse_x || g_env.envInputs.keys[DIK_LEFTARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeX > dt )
		{
			pos.x -= CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else
	if( g_render->GetWidth() - 1 == g_env.envInputs.mouse_x || g_env.envInputs.keys[DIK_RIGHTARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeX > dt )
		{
			pos.x += CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else
		_dwTimeX = GetTickCount();
	//---------------------------------------
	if( 0 == g_env.envInputs.mouse_y || g_env.envInputs.keys[DIK_UPARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			pos.y -= CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else
	if( g_render->GetHeight()-1 == g_env.envInputs.mouse_y || g_env.envInputs.keys[DIK_DOWNARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			pos.y += CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else
		_dwTimeY = GetTickCount();
	//---------------------------------------
	if( bMove )
		_dt = __max(10.0f, 1.0f / (1.0f / _dt + 0.001f));
	else
		_dt = 50.0f;
	//------------------------------------------------------
	int dx = __max(0, (int) ((float)  WIDTH(_viewport) / _zoom - g_level->_sx) / 2);
	int dy = __max(0, (int) ((float) HEIGHT(_viewport) / _zoom - g_level->_sy) / 2);
	pos.x = (float) __max(int(pos.x), dx);
	pos.x = (float) __min(int(pos.x), int(g_level->_sx - (float) WIDTH(_viewport) / _zoom) + dx);
	pos.y = (float) __max(int(pos.y), dy);
	pos.y = (float) __min(int(pos.y), int(g_level->_sy - (float) HEIGHT(_viewport) / _zoom) + dy);

	MoveTo(pos);
}

void GC_Camera::SwitchEditor()
{
	UpdateLayout();
}

void GC_Camera::UpdateLayout()
{
	GC_Camera *tmp = NULL;
	size_t active_count = 0;

	FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
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
	}

	if( tmp && 0 == active_count )
	{
		tmp->Activate(true);
		++active_count;
	}

	RECT viewports[MAX_HUMANS];

	if( g_render->GetWidth() >= (int)g_level->_sx &&
		g_render->GetHeight() >= (int)g_level->_sy )
	{
		SetRect(&viewports[0],
			(g_render->GetWidth() - (int) g_level->_sx) / 2,
			(g_render->GetHeight() - (int) g_level->_sy) / 2,
			(g_render->GetWidth() + (int) g_level->_sx) / 2,
			(g_render->GetHeight() + (int) g_level->_sy) / 2
		);
		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera)
		{
			pCamera->_viewport = viewports[0];
		}
	}
	else
	{
		if( 0 == active_count ) return;

		int w = g_render->GetWidth();
		int h = g_render->GetHeight();

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
			assert(false);
		}

		size_t count = 0;
		float  zoom  = active_count > 2 ? 0.5f : 1.0f;
		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
		{
			if( !pCamera->IsActive() ) continue;
			pCamera->_viewport = viewports[count++];
			pCamera->_zoom     = zoom;
		}
	}
}

bool GC_Camera::GetWorldMousePos(vec2d &pos)
{
	POINT ptinscr = { g_env.envInputs.mouse_x, g_env.envInputs.mouse_y };

	FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
	{
		if( !pCamera->IsActive() ) continue;
		if( PtInRect(&pCamera->_viewport, ptinscr) )
		{
			pCamera->Select();
			pos.x = (float) (ptinscr.x - pCamera->_viewport.left) / pCamera->_zoom + (float) g_env.camera_x;
            pos.y = (float) (ptinscr.y - pCamera->_viewport.top) / pCamera->_zoom + (float) g_env.camera_y;
			return true;
		}
	}
	return false;
}

void GC_Camera::Shake(float level)
{
	assert(_player);
	if( 0 == _time_shake )
		_time_seed = frand(1000.0f);
	_time_shake = __min(_time_shake + 0.5f * level, PLAYER_RESPAWNTIME / 2);
}

void GC_Camera::Serialize(SaveFile &f)
{
	GC_Actor::Serialize(f);

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

	_rotator.Serialize(f);
	if( f.loading() ) UpdateLayout();
}

void GC_Camera::Kill()
{
	_player = NULL;
	Activate(false);
	//-------------------
	GC_Actor::Kill();
	//-------------------
	UpdateLayout();
}

void GC_Camera::OnDetach(GC_Object *sender, void *param)
{
	Kill();
}



// end of file
