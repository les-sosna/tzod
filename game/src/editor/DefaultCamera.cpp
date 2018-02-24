#include "inc/editor/detail/DefaultCamera.h"
#include <gc/WorldCfg.h>
#include <ui/Keys.h>
#include <ui/UIInput.h>
#include <chrono>
#include <algorithm>

//                              0        1       2      3     4     5     6
static float s_zoomLevels[] = { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };

DefaultCamera::DefaultCamera(vec2d pos)
	: _zoomLevel(4)
	, _zoom(1)
	, _pos(pos)
{
}

void DefaultCamera::Move(vec2d offset, const FRECT &worldBounds)
{
	_pos = Vec2dClamp(_pos - offset * 30, worldBounds);
}

void DefaultCamera::ZoomIn()
{
	_zoomLevel = std::min(_zoomLevel + 1, (int)(sizeof(s_zoomLevels) / sizeof(float)) - 1);
}

void DefaultCamera::ZoomOut()
{
	_zoomLevel = std::max(_zoomLevel - 1, 0);
}

void DefaultCamera::HandleMovement(UI::IInput &input, const FRECT &worldBounds, float dt)
{
	vec2d direction = {};

	if (input.IsKeyPressed(UI::Key::A))
		direction.x = -1;
	else if (input.IsKeyPressed(UI::Key::D))
		direction.x = 1;

	if (input.IsKeyPressed(UI::Key::W))
		direction.y = -1;
	else if (input.IsKeyPressed(UI::Key::S))
		direction.y = 1;

	direction.Normalize();

	if (input.GetGamepadState(0).rightThumbstickPos.sqr() > .5f)
	{
		direction += input.GetGamepadState(0).rightThumbstickPos;
	}

	float targetZoom = s_zoomLevels[_zoomLevel];
	if (_zoom != targetZoom)
	{
		float exp = std::exp(-dt * 5);
		float delta = (_zoom - targetZoom) * exp * 0.8f;
		if (_zoom > targetZoom)
			_zoom = std::max(targetZoom, targetZoom + delta);
		else
			_zoom = std::min(targetZoom, targetZoom + delta);
	}

	_speed += direction * dt * (6000 + _speed.len() * 8);
	_speed *= expf(-dt * 10);

	_pos += _speed * dt / _zoom;
	_pos = Vec2dClamp(_pos, worldBounds);

	if (input.IsKeyPressed(UI::Key::Home))
	{
		_pos = vec2d{};
		_speed = vec2d{};
	}
}
