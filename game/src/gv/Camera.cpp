#include "inc/gv/Camera.h"

#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/Weapons.h>
#include <gc/WorldCfg.h>
#include <gc/World.h>

Camera::Camera(vec2d pos, GC_Player &player)
	: _player(player)
	, _pos(pos)
	, _target(pos)
	, _time_shake(0)
	, _time_seed(frand(1000))
{
}

void Camera::CameraTimeStep(World &world, float dt, float scale)
{
	const GC_Vehicle *vehicle = _player.GetVehicle();
	vec2d viewSize = vec2d{ (float)WIDTH(_viewport), (float)HEIGHT(_viewport) } / scale;

	float mu = 3;

	if( vehicle )
	{
		mu += vehicle->_lv.len() / 100;

		float dx = std::max(0.f, (viewSize.x - WIDTH(world._bounds)) / 2);
		float dy = std::max(0.f, (viewSize.y - HEIGHT(world._bounds)) / 2);

		vec2d r = vehicle->GetPos() + vehicle->_lv / mu;
		float directionMultipler = std::min(130.0f, std::min(viewSize.x, viewSize.y) / 3);

		if( GC_Weapon *weapon = vehicle->GetWeapon() )
			r += weapon->GetDirection() * directionMultipler;
		else
			r += vehicle->GetDirection() * directionMultipler;

		_target.x = std::max(r.x + dx, world._bounds.left + viewSize.x * 0.5f + dx);
		_target.y = std::max(r.y + dy, world._bounds.top + viewSize.y * 0.5f + dy);
		_target.x = std::min(_target.x, world._bounds.right - viewSize.x * 0.5f + dx);
		_target.y = std::min(_target.y, world._bounds.bottom - viewSize.y * 0.5f + dy);
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
	vec2d shake{0, 0};
	if( _time_shake > 0 )
	{
		shake.x = std::cos((_time_shake + _time_seed)*70.71068f);
		shake.y = std::sin((_time_shake + _time_seed)*86.60254f);
		shake *= _time_shake * WORLD_BLOCK_SIZE * 0.1f;
	}
	return _pos + shake;
}

void Camera::Shake(float level)
{
	if( 0 == _time_shake )
		_time_seed = frand(1000.0f);
	_time_shake = std::min(_time_shake + 0.5f * level, 1.0f);
}
