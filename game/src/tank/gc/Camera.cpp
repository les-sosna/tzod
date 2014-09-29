// Camera.cpp

#include "Camera.h"
#include "World.h"
#include "Macros.h"
#include "Player.h"
#include "Vehicle.h"
#include "Weapons.h"

#include "constants.h"
#include "SaveFile.h"
#include "config/Config.h"

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
  : _player(player)
{
	assert(_player);

	MoveTo(world, vec2d(world._sx / 2, world._sy / 2));
	if( _player->GetVehicle() )
	{
		MoveTo(world, _player->GetVehicle()->GetPos());
	}
	_player->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Camera::OnDetach);

	_target     = GetPos();
	_time_shake = 0;
	_time_seed  = frand(1000);
	_zoom       = 1.0f;
}

GC_Camera::GC_Camera(FromFile)
{
}

GC_Camera::~GC_Camera()
{
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

	if( _player->GetVehicle() )
	{
		mu += _player->GetVehicle()->_lv.len() / 100;

		int dx = (int) std::max(.0f, (viewSize.x / _zoom - world._sx) / 2);
		int dy = (int) std::max(.0f, (viewSize.y / _zoom - world._sy) / 2);

		vec2d r = _player->GetVehicle()->GetPos() + _player->GetVehicle()->_lv / mu;
		float directionMultipler = std::min(130.0f, std::min(viewSize.x, viewSize.y) / 3);

		if( _player->GetVehicle()->GetWeapon() )
		{
			r += _player->GetVehicle()->GetWeapon()->GetDirection() * directionMultipler;
		}
		else
		{
			r += _player->GetVehicle()->GetDirection() * directionMultipler;
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

vec2d GC_Camera::GetCameraPos() const
{
	vec2d shake(0, 0);
	if( _time_shake > 0 )
	{
		shake.Set(cos((_time_shake + _time_seed)*70.71068f), sin((_time_shake + _time_seed)*86.60254f));
		shake *= _time_shake * CELL_SIZE * 0.1f;
	}
	return GetPos() + shake;
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

	f.Serialize(_target);
	f.Serialize(_time_seed);
	f.Serialize(_time_shake);
	f.Serialize(_zoom);
	f.Serialize(_player);
}

void GC_Camera::OnDetach(World &world, GC_Object *sender, void *param)
{
	Kill(world);
}

// end of file
