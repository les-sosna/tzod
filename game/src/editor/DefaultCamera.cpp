#include "inc/editor/detail/DefaultCamera.h"
#include <gc/WorldCfg.h>
#include <ui/Keys.h>
#include <ui/UIInput.h>
#include <chrono>
#include <algorithm>

//                              0        1       2      3     4     5     6
static float s_zoomLevels[] = { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f };

DefaultCamera::DefaultCamera(vec2d pos)
	: _zoom(1)
	, _zoomLevel(4)
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
	_zoom = s_zoomLevels[_zoomLevel];
}

void DefaultCamera::ZoomOut()
{
	_zoomLevel = std::max(_zoomLevel - 1, 0);
	_zoom = s_zoomLevels[_zoomLevel];
}

void DefaultCamera::HandleMovement(UI::IInput &input, const FRECT &worldBounds, float dt)
{
	static char  lastIn = 0, LastOut = 0;

	if (!lastIn && (input.IsKeyPressed(UI::Key::PageUp)))
		ZoomIn();
	lastIn = input.IsKeyPressed(UI::Key::PageUp);

	if (!LastOut && (input.IsKeyPressed(UI::Key::PageDown)))
		ZoomOut();
	LastOut = input.IsKeyPressed(UI::Key::PageDown);

	vec2d direction = {};

	if (input.IsKeyPressed(UI::Key::Left))
		direction.x = -1;
	else if (input.IsKeyPressed(UI::Key::Right))
		direction.x = 1;

	if (input.IsKeyPressed(UI::Key::Up))
		direction.y = -1;
	else if (input.IsKeyPressed(UI::Key::Down))
		direction.y = 1;

	direction.Normalize();

	if (input.GetGamepadState(0).rightThumbstickPos.sqr() > .5f)
	{
		direction += input.GetGamepadState(0).rightThumbstickPos;
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
