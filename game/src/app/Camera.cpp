#include "inc/app/Camera.h"

#include <gc/Vehicle.h>
#include <gc/Weapons.h>
#include <gc/WorldCfg.h>
#include <gc/SaveFile.h>
#include <gc/World.h>

Camera::Camera(vec2d pos, unsigned int index)
  : _index(index)
  , _pos(pos)
  , _target(pos)
  , _time_shake(0)
  , _time_seed(frand(1000))
{
}

void Camera::CameraTimeStep(World &world, const GC_Vehicle *vehicle, float dt)
{
	vec2d viewSize((float) WIDTH(_viewport), (float) HEIGHT(_viewport));

	float mu = 3;

	if( vehicle )
	{
		mu += vehicle->_lv.len() / 100;

		int dx = (int) std::max(.0f, (viewSize.x - world._sx) / 2);
		int dy = (int) std::max(.0f, (viewSize.y - world._sy) / 2);

		vec2d r = vehicle->GetPos() + vehicle->_lv / mu;
		float directionMultipler = std::min(130.0f, std::min(viewSize.x, viewSize.y) / 3);

		if( GC_Weapon *weapon = vehicle->GetWeapon() )
			r += weapon->GetDirection() * directionMultipler;
		else
			r += vehicle->GetDirection() * directionMultipler;

		_target.x = std::max(r.x + (float) dx, viewSize.x * 0.5f + dx);
		_target.y = std::max(r.y + (float) dy, viewSize.y * 0.5f + dy);
		_target.x = std::min(_target.x, world._sx - viewSize.x * 0.5f + dx);
		_target.y = std::min(_target.y, world._sy - viewSize.y * 0.5f + dy);
	}

	if( _time_shake > 0 )
	{
		_time_shake -= dt;
		if( _time_shake < 0 )
			_time_shake = 0;
	}

	_pos = _target + (_pos - _target) * expf(-dt * mu);
}

vec2d Camera::GetCameraPos() const
{
	vec2d shake(0, 0);
	if( _time_shake > 0 )
	{
		shake.Set(cos((_time_shake + _time_seed)*70.71068f), sin((_time_shake + _time_seed)*86.60254f));
		shake *= _time_shake * CELL_SIZE * 0.1f;
	}
	return _pos + shake;
}

void Camera::Shake(float level)
{
	if( 0 == _time_shake )
		_time_seed = frand(1000.0f);
	_time_shake = std::min(_time_shake + 0.5f * level, 1.0f);
}

void Camera::Serialize(World &world, SaveFile &f)
{
	f.Serialize(_target);
	f.Serialize(_time_seed);
	f.Serialize(_time_shake);
}
