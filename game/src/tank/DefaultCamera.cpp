// DefaultCamera.cpp

#include "stdafx.h"
#include "DefaultCamera.h"
#include "globals.h"
#include "video/RenderBase.h"

DefaultCamera::DefaultCamera()
  : _zoom(1)
  , _dt(50)
  , _pos(0,0)
{
	_dwTimeX = _dwTimeY = GetTickCount();
}

void DefaultCamera::HandleMovement(float worldWidth, float worldHeight, 
                                   float screenWidth, float screenHeight)
{
	static char  lastIn   = 0, LastOut = 0;
	static float levels[] = { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f };
	static int   level    = 4;

	if( !lastIn && g_env.envInputs.keys[DIK_PGUP] )
		level = __min(level+1, sizeof(levels) / sizeof(float) - 1);
	lastIn = g_env.envInputs.keys[DIK_PGUP];

	if( !LastOut && g_env.envInputs.keys[DIK_PGDN] )
		level = __max(level-1, 0);
	LastOut = g_env.envInputs.keys[DIK_PGDN];

	_zoom = levels[level];

	bool  bMove     = false;
	DWORD dwCurTime = GetTickCount();
	DWORD dt        = DWORD(_dt);

	if( 0 == g_env.envInputs.mouse_x || g_env.envInputs.keys[DIK_LEFTARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeX > dt )
		{
			_pos.x -= CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else
	if( g_render->GetWidth() - 1 == g_env.envInputs.mouse_x || g_env.envInputs.keys[DIK_RIGHTARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeX > dt )
		{
			_pos.x += CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else
		_dwTimeX = GetTickCount();
	//---------------------------------------
	if( 0 == g_env.envInputs.mouse_y || g_env.envInputs.keys[DIK_UPARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			_pos.y -= CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else
	if( g_render->GetHeight()-1 == g_env.envInputs.mouse_y || g_env.envInputs.keys[DIK_DOWNARROW] )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			_pos.y += CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else
		_dwTimeY = GetTickCount();
	//---------------------------------------
	if( bMove )
		_dt = __max(10.0f, 1.0f / (1.0f / _dt + 0.001f));
	else
		_dt = 50.0f;
	//------------------------------------------------------
	int dx = __max(0, (int) (screenWidth / _zoom - worldWidth) / 2);
	int dy = __max(0, (int) (screenHeight / _zoom - worldHeight) / 2);
	_pos.x = (float) __max(int(_pos.x), dx);
	_pos.x = (float) __min(int(_pos.x), int(worldWidth - screenWidth / _zoom) + dx);
	_pos.y = (float) __max(int(_pos.y), dy);
	_pos.y = (float) __min(int(_pos.y), int(worldHeight - screenHeight / _zoom) + dy);
}

// end of file
