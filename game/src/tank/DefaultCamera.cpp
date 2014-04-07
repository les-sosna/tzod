// DefaultCamera.cpp

#include "DefaultCamera.h"
#include "globals.h"
#include "constants.h"

#include <chrono>
#include <algorithm>

#include <GLFW/glfw3.h>


static unsigned int GetMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

static bool IsKeyPressed(int key)
{
    return GLFW_PRESS == glfwGetKey(g_appWindow, key);
}


DefaultCamera::DefaultCamera()
  : _zoom(1)
  , _dt(50)
  , _pos(0,0)
{
	_dwTimeX = _dwTimeY = GetMilliseconds();
}

void DefaultCamera::HandleMovement(float worldWidth, float worldHeight, 
                                   float screenWidth, float screenHeight)
{
	static char  lastIn   = 0, LastOut = 0;
	static float levels[] = { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f };
	static int   level    = 4;
    
	if( !lastIn && IsKeyPressed(GLFW_KEY_PAGE_UP) )
		level = std::min(level+1, (int) (sizeof(levels) / sizeof(float)) - 1);
	lastIn = IsKeyPressed(GLFW_KEY_PAGE_UP);

	if( !LastOut && IsKeyPressed(GLFW_KEY_PAGE_DOWN) )
		level = std::max(level - 1, 0);
	LastOut = IsKeyPressed(GLFW_KEY_PAGE_DOWN);

	_zoom = levels[level];

	bool  bMove     = false;
	unsigned int dwCurTime = GetMilliseconds();
	unsigned int dt        = _dt;
    
    double mouse_x = 0, mouse_y = 0;
    glfwGetCursorPos(g_appWindow, &mouse_x, &mouse_y);

	if( 0 == (int) mouse_x || IsKeyPressed(GLFW_KEY_LEFT) )
	{
		bMove = true;
		while( dwCurTime - _dwTimeX > dt )
		{
			_pos.x -= CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else
	if( screenWidth - 1 == (int) mouse_x || IsKeyPressed(GLFW_KEY_RIGHT) )
	{
		bMove = true;
		while( dwCurTime - _dwTimeX > dt )
		{
			_pos.x += CELL_SIZE;
			_dwTimeX += dt;
		}
	}
	else
		_dwTimeX = GetMilliseconds();
	//---------------------------------------
	if( 0 == (int) mouse_y || IsKeyPressed(GLFW_KEY_UP) )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			_pos.y -= CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else
	if( screenHeight - 1 == (int) mouse_y || IsKeyPressed(GLFW_KEY_DOWN) )
	{
		bMove = true;
		while( dwCurTime - _dwTimeY > dt )
		{
			_pos.y += CELL_SIZE;
			_dwTimeY += dt;
		}
	}
	else
		_dwTimeY = GetMilliseconds();
	//---------------------------------------
	if( bMove )
		_dt = std::max(10.0f, 1.0f / (1.0f / _dt + 0.001f));
	else
		_dt = 50.0f;
	//------------------------------------------------------
	int dx = std::max(0, (int) (screenWidth / _zoom - worldWidth) / 2);
	int dy = std::max(0, (int) (screenHeight / _zoom - worldHeight) / 2);
	_pos.x = (float) std::max(int(_pos.x), dx);
	_pos.x = (float) std::min(int(_pos.x), int(worldWidth - screenWidth / _zoom) + dx);
	_pos.y = (float) std::max(int(_pos.y), dy);
	_pos.y = (float) std::min(int(_pos.y), int(worldHeight - screenHeight / _zoom) + dy);
}

// end of file
