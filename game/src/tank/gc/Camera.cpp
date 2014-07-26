// Camera.cpp

#include "Camera.h"

#include "World.h"
#include "Macros.h"
#include "Player.h"
#include "SaveFile.h"
#include "Vehicle.h"
#include "Weapons.h"

#include "constants.h"
#include "config/Config.h"
#include "video/RenderBase.h" // FIXME

// ui
#include <ConsoleBuffer.h>
UI::ConsoleBuffer& GetConsole();

#ifndef NOSOUND
#include <al.h>
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Camera)
{
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_Camera, LIST_cameras);

GC_Camera::GC_Camera(World &world, GC_Player *player)
  : _rotator(_rotatorAngle)
  , _player(player)
{
	assert(_player);

	_rotator.reset(0.0f, 0.0f,
		g_conf.g_rotcamera_m.GetFloat(),
		g_conf.g_rotcamera_a.GetFloat(),
		std::max(0.001f, g_conf.g_rotcamera_s.GetFloat()));

	MoveTo(world, vec2d(world._sx / 2, world._sy / 2));
	if( _player->GetVehicle() )
	{
		_rotatorAngle =  -_player->GetVehicle()->GetDirection().Angle() + PI/2;
		MoveTo(world, _player->GetVehicle()->GetPos());
	}
	_player->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Camera::OnDetach);

	_target     = GetPos();
	_time_shake = 0;
	_time_seed  = frand(1000);
	_zoom       = 1.0f;
}

GC_Camera::GC_Camera(FromFile)
  : _rotator(_rotatorAngle)
{
}

GC_Camera::~GC_Camera()
{
}

void GC_Camera::Kill(World &world)
{
    GC_Actor::Kill(world);
}

void GC_Camera::MoveTo(World &world, const vec2d &pos)
{
#ifndef NOSOUND
    alListener3f(AL_POSITION, GetPos().x, GetPos().y, 500.0f);
#endif
    GC_Actor::MoveTo(world, pos);
}

void GC_Camera::CameraTimeStep(World &world, float dt, vec2d viewSize)
{
	float mu = 3;

	_rotator.process_dt(dt);
	if( _player->GetVehicle() )
	{
		_rotator.rotate_to(-_player->GetVehicle()->GetDirection().Angle() - PI/2);

		mu += _player->GetVehicle()->_lv.len() / 100;

		int dx = (int) std::max(.0f, (viewSize.x / _zoom - world._sx) / 2);
		int dy = (int) std::max(.0f, (viewSize.y / _zoom - world._sy) / 2);

		vec2d r = _player->GetVehicle()->GetPos() + _player->GetVehicle()->_lv / mu;

		if( _player->GetVehicle()->GetWeapon() )
		{
			r += _player->GetVehicle()->GetWeapon()->GetDirection() * 130.0f;
		}
		else
		{
			r += _player->GetVehicle()->GetDirection() * 130.0f;
		}

		_target.x = r.x + (float) dx;
		_target.y = r.y + (float) dy;

		_target.x = std::max(_target.x, viewSize.x / _zoom * 0.5f + dx);
		_target.x = std::min(_target.x, world._sx - viewSize.x / _zoom * 0.5f + dx);
		_target.y = std::max(_target.y, viewSize.y / _zoom * 0.5f + dy);
		_target.y = std::min(_target.y, world._sy - viewSize.y / _zoom * 0.5f + dy);
	}

	if( _time_shake > 0 )
	{
		_time_shake -= dt;
		if( _time_shake < 0 )
			_time_shake = 0;
	}

	MoveTo(world, _target + (GetPos() - _target) * expf(-dt * mu));
}

void GC_Camera::GetWorld(FRECT &outWorld, const Rect &screen) const
{
	vec2d shake(0, 0);
	if( _time_shake > 0 )
	{
		shake.Set(cos((_time_shake + _time_seed)*70.71068f), sin((_time_shake + _time_seed)*86.60254f));
		shake *= _time_shake * CELL_SIZE * 0.1f;
	}

	outWorld.left   = floor((GetPos().x + shake.x - (float)  WIDTH(screen) / _zoom * 0.5f) * _zoom) / _zoom;
	outWorld.top    = floor((GetPos().y + shake.y - (float) HEIGHT(screen) / _zoom * 0.5f) * _zoom) / _zoom;
	outWorld.right  = outWorld.left + (float)  WIDTH(screen) / _zoom;
	outWorld.bottom = outWorld.top + (float) HEIGHT(screen) / _zoom;
}

void GC_Camera::Shake(float level)
{
	assert(_player);
	if( 0 == _time_shake )
		_time_seed = frand(1000.0f);
	_time_shake = std::min(_time_shake + 0.5f * level, PLAYER_RESPAWN_DELAY / 2);
}

void GC_Camera::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);

	f.Serialize(_rotatorAngle);
	f.Serialize(_target);
	f.Serialize(_time_seed);
	f.Serialize(_time_shake);
	f.Serialize(_zoom);
	f.Serialize(_player);

	_rotator.Serialize(f);
}

void GC_Camera::OnDetach(World &world, GC_Object *sender, void *param)
{
	Kill(world);
}

// end of file
