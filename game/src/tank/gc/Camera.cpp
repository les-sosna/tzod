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

GC_Camera::GC_Camera(const SafePtr<GC_Player> &player)
  : GC_Actor()
  , _memberOf(this)
  , _rotator(_rotatorAngle)
  , _player(player)
{
	assert(_player);

	_rotator.reset(0.0f, 0.0f,
		g_conf.g_rotcamera_m.GetFloat(),
		g_conf.g_rotcamera_a.GetFloat(),
		__max(0.001f, g_conf.g_rotcamera_s.GetFloat()));

	MoveTo( vec2d(g_level->_sx / 2, g_level->_sy / 2) );
	if( _player->GetVehicle() )
	{
		_rotatorAngle =  -_player->GetVehicle()->GetVisual()->GetDirection().Angle() + PI/2;
		MoveTo( _player->GetVehicle()->GetPosPredicted() );
	}
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
	_player->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Camera::OnDetach);
	_player->Subscribe(NOTIFY_PLAYER_SETCONTROLLER, this, (NOTIFYPROC) &GC_Camera::OnDetach);

	_target     = GetPos();
	_time_shake = 0;
	_time_seed  = frand(1000);
	_zoom       = 1.0f;

	UpdateLayout();
}

GC_Camera::GC_Camera(FromFile)
  : GC_Actor(FromFile())
  , _memberOf(this)
  , _rotator(_rotatorAngle)
{
}

void GC_Camera::TimeStepFloat(float dt)
{
	float mu = 3;

	_rotator.process_dt(dt);
	if( _player->GetVehicle() )
	{
		_rotator.rotate_to(-_player->GetVehicle()->GetVisual()->GetDirection().Angle() - PI/2);

		mu += _player->GetVehicle()->GetVisual()->_lv.len() / 100;

		int dx = (int) __max(0, ((float) WIDTH(_viewport) / _zoom  - g_level->_sx) / 2);
		int dy = (int) __max(0, ((float) HEIGHT(_viewport) / _zoom - g_level->_sy) / 2);

		vec2d r = _player->GetVehicle()->GetPosPredicted() + _player->GetVehicle()->GetVisual()->_lv / mu;

		if( _player->GetVehicle()->GetWeapon() )
		{
			r += _player->GetVehicle()->GetWeapon()->GetDirection() * 130.0f;
		}
		else
		{
			r += _player->GetVehicle()->GetVisual()->GetDirection() * 130.0f;
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

void GC_Camera::GetWorld(FRECT &outWorld) const
{
	vec2d shake(0, 0);
	if( _time_shake > 0 )
	{
		shake.Set(cos((_time_shake + _time_seed)*70.71068f), sin((_time_shake + _time_seed)*86.60254f));
		shake *= _time_shake * CELL_SIZE * 0.1f;
	}

	outWorld.left   = floor((GetPos().x + shake.x - (float)  WIDTH(_viewport) / _zoom * 0.5f) * _zoom) / _zoom;
	outWorld.top    = floor((GetPos().y + shake.y - (float) HEIGHT(_viewport) / _zoom * 0.5f) * _zoom) / _zoom;
	outWorld.right  = outWorld.left + (float)  WIDTH(_viewport) / _zoom;
	outWorld.bottom = outWorld.top + (float) HEIGHT(_viewport) / _zoom;
}

void GC_Camera::GetScreen(RECT &vp) const
{
	vp = _viewport;
}

void GC_Camera::UpdateLayout()
{
	GC_Camera *any = NULL;
	size_t camCount = 0;

	FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
	{
		if( !pCamera->IsKilled() )
		{
			any = pCamera;
			++camCount;
		}
	}

	RECT viewports[MAX_HUMANS];

	if( g_render->GetWidth() >= int(g_level->_sx) && g_render->GetHeight() >= int(g_level->_sy) )
	{
		SetRect(&viewports[0],
			(g_render->GetWidth() - int(g_level->_sx)) / 2,
			(g_render->GetHeight() - int(g_level->_sy)) / 2,
			(g_render->GetWidth() + int(g_level->_sx)) / 2,
			(g_render->GetHeight() + int(g_level->_sy)) / 2
		);
		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera)
		{
			pCamera->_viewport = viewports[0];
		}
	}
	else if( camCount )
	{
		int w = g_render->GetWidth();
		int h = g_render->GetHeight();

		switch( camCount )
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
		float  zoom  = camCount > 2 ? 0.5f : 1.0f;
		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
		{
			if( !pCamera->IsKilled() )
			{
				pCamera->_viewport = viewports[count++];
				pCamera->_zoom     = zoom;
			}
		}
	}
}

bool GC_Camera::GetWorldMousePos(vec2d &pos)
{
	POINT ptinscr = { g_env.envInputs.mouse_x, g_env.envInputs.mouse_y };

	if( g_level->GetEditorMode() || g_level->GetList(LIST_cameras).empty() )
	{
		// use default camera
		pos.x = float(ptinscr.x) / g_level->_defaultCamera.GetZoom() + g_level->_defaultCamera.GetPosX();
		pos.y = float(ptinscr.y) / g_level->_defaultCamera.GetZoom() + g_level->_defaultCamera.GetPosY();
		return true;
	}
	else
	{
		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
		{
			if( !pCamera->IsKilled() && PtInRect(&pCamera->_viewport, ptinscr) )
			{
				FRECT w;
				pCamera->GetWorld(w);
				pos.x = w.left + (float) (ptinscr.x - pCamera->_viewport.left) / pCamera->_zoom;
				pos.y = w.top + (float) (ptinscr.y - pCamera->_viewport.top) / pCamera->_zoom;
				return true;
			}
		}
	}
	return false;
}

void GC_Camera::Shake(float level)
{
	assert(_player);
	if( 0 == _time_shake )
		_time_seed = frand(1000.0f);
	_time_shake = __min(_time_shake + 0.5f * level, PLAYER_RESPAWN_DELAY / 2);
}

void GC_Camera::Serialize(SaveFile &f)
{
	GC_Actor::Serialize(f);

	f.Serialize(_rotatorAngle);
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
	GC_Actor::Kill();
	UpdateLayout();
}

void GC_Camera::OnDetach(GC_Object *sender, void *param)
{
	Kill();
}

// end of file
