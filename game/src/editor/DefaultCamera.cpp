#include "inc/editor/detail/DefaultCamera.h"
#include <gc/WorldCfg.h>
#include <plat/Input.h>
#include <plat/Keys.h>
#include <chrono>
#include <algorithm>

//                              0        1       2      3     4     5     6
static float s_zoomLevels[] = { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };

DefaultCamera::DefaultCamera(vec2d pos)
	: _pos(pos)
{
}

void DefaultCamera::MoveTo(vec2d newEyeWorldPos)
{
	_speed = vec2d{};
	_targetPos = newEyeWorldPos;
	_movingToTarget = true;
}

void DefaultCamera::Move(vec2d offset, const FRECT &worldBounds)
{
	_pos = Vec2dClamp(_pos - offset, worldBounds);
}

void DefaultCamera::ZoomIn()
{
	_zoomLevel = std::min(_zoomLevel + 1, (int)(sizeof(s_zoomLevels) / sizeof(float)) - 1);
}

void DefaultCamera::ZoomOut()
{
	_zoomLevel = std::max(_zoomLevel - 1, 0);
}

void DefaultCamera::HandleMovement(Plat::Input &input, const FRECT &worldBounds, float dt)
{
	vec2d direction = {};

	if (input.IsKeyPressed(Plat::Key::A))
		direction.x = -1;
	else if (input.IsKeyPressed(Plat::Key::D))
		direction.x = 1;

	if (input.IsKeyPressed(Plat::Key::W))
		direction.y = -1;
	else if (input.IsKeyPressed(Plat::Key::S))
		direction.y = 1;

	direction.Normalize();

	if (input.GetGamepadState(0).rightThumbstickPos.sqr() > .5f)
	{
		direction += input.GetGamepadState(0).rightThumbstickPos;
	}

	// Pure exponent would move to target infinitely long.
	// Adjust target to be slightly ahead of the desired pos.
	// This way we reach target quicker but we need to explicitly
	// stop in order to not pass beyond.

	float expFactor = 5;
	float targetPosPreemption = 10; // distance units
	vec2d posDifference = _targetPos - _pos;
	vec2d targetDirection = posDifference.Norm();
	vec2d adjustedTragetPos = _targetPos + targetDirection * targetPosPreemption;
	vec2d adjustedDifference = adjustedTragetPos - _pos;

	// keep the speed for smooth transition
	if (_movingToTarget && !direction.IsZero())
	{
		_movingToTarget = false;
		_speed = adjustedDifference * expFactor;
	}

	float exp = std::exp(-dt * expFactor);
	float targetZoom = s_zoomLevels[_zoomLevel];
	if (_zoom != targetZoom)
	{
		// FIXME: const factor seems to be wrong. Use adjusted target zoom instead.
		float reminder = (targetZoom - _zoom) * exp * 0.8f;
		if (_zoom > targetZoom)
			_zoom = std::max(targetZoom, targetZoom - reminder);
		else
			_zoom = std::min(targetZoom, targetZoom - reminder);
	}

	if (_movingToTarget)
	{
		vec2d reminder = adjustedDifference * exp;
		if (reminder.sqr() > targetPosPreemption * targetPosPreemption)
		{
			_pos = adjustedTragetPos - reminder;
		}
		else
		{
			_pos = _targetPos;
			_movingToTarget = false;
		}
	}
	else
	{
		_speed += direction * dt * (6000 + _speed.len() * 8);
		_speed *= expf(-dt * 10);

		_pos += _speed * dt / _zoom;
		_pos = Vec2dClamp(_pos, worldBounds);
	}


	if (input.IsKeyPressed(Plat::Key::Home))
	{
		_targetPos = vec2d{};
		_speed = vec2d{};
		_movingToTarget = true;
	}
}
