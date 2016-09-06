#include "DefaultCamera.h"
#include <gc/WorldCfg.h>
#include <ui/Keys.h>
#include <ui/UIInput.h>
#include <chrono>
#include <algorithm>


static unsigned int GetMilliseconds()
{
	using namespace std::chrono;
	return duration_cast<duration<unsigned int, std::milli>>(high_resolution_clock::now().time_since_epoch()).count();
}

DefaultCamera::DefaultCamera(vec2d pos)
	: _zoom(1)
	, _dt(50)
	, _pos(pos)
{
	_dwTimeX = _dwTimeY = GetMilliseconds();
}

void DefaultCamera::Move(vec2d offset, const FRECT &worldBounds)
{
    _pos = Vec2dConstrain(_pos - offset * 30, worldBounds);
}

void DefaultCamera::HandleMovement(UI::IInput &input, const FRECT &worldBounds)
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

	bool  bMove     = false;
	unsigned int dwCurTime = GetMilliseconds();
	unsigned int dt        = (unsigned int) _dt;

	if( input.IsKeyPressed(UI::Key::Left) )
	{
		bMove = true;
		while( dwCurTime - _dwTimeX > dt )
		{
			_pos.x -= CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else if( input.IsKeyPressed(UI::Key::Right) )
	{
		bMove = true;
		while( dwCurTime - _dwTimeX > dt )
		{
			_pos.x += CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else
	{
		_dwTimeX = GetMilliseconds();
	}

	if( input.IsKeyPressed(UI::Key::Up) )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			_pos.y -= CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else if( input.IsKeyPressed(UI::Key::Down) )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			_pos.y += CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else
	{
		_dwTimeY = GetMilliseconds();
	}

	if( bMove )
		_dt = std::max(10.0f, 1.0f / (1.0f / _dt + 0.001f));
	else
		_dt = 50.0f;

	_pos = Vec2dConstrain(_pos, worldBounds);

	if (input.IsKeyPressed(UI::Key::Home))
	{
		_pos = vec2d{};
	}
}
