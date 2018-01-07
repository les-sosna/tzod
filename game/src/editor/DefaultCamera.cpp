#include "inc/editor/detail/DefaultCamera.h"
#include <gc/WorldCfg.h>
#include <ui/Keys.h>
#include <ui/UIInput.h>
#include <chrono>
#include <algorithm>

DefaultCamera::DefaultCamera(vec2d pos)
	: _zoom(1)
	, _pos(pos)
{
}

void DefaultCamera::Move(vec2d offset, const FRECT &worldBounds)
{
	_pos = Vec2dClamp(_pos - offset * 30, worldBounds);
}

void DefaultCamera::HandleMovement(UI::IInput &input, const FRECT &worldBounds, float dt)
{
	static char  lastIn   = 0, LastOut = 0;
	static float levels[] = { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f };
	static int   level    = 4;

	if( !lastIn && input.IsKeyPressed(UI::Key::PageUp) )
		level = std::min(level+1, (int) (sizeof(levels) / sizeof(float)) - 1);
	lastIn = input.IsKeyPressed(UI::Key::PageUp);

	if( !LastOut && input.IsKeyPressed(UI::Key::PageDown) )
		level = std::max(level - 1, 0);
	LastOut = input.IsKeyPressed(UI::Key::PageDown);

	_zoom = levels[level];

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
