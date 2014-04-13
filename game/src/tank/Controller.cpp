// Controller.cpp

#include "Controller.h"
#include "config/Config.h"
#include "network/ControlPacket.h" // for VehicleState
#include "gc/Camera.h"
#include "gc/Sound.h"
#include "gc/Vehicle.h"
#include "Level.h"
#include "KeyMapper.h"

#include <GLFW/glfw3.h>
#include <ConsoleBuffer.h>
UI::ConsoleBuffer& GetConsole();


static bool IsKeyPressed(int key)
{
    return GLFW_PRESS == glfwGetKey(g_appWindow, key);
}

static bool IsMousePressed(int button)
{
    return GLFW_PRESS == glfwGetMouseButton(g_appWindow, button);
}


Controller::Controller()
  : _lastLightKeyState(false)
  , _lastLightsState(true)
{
	_keyForward     = 0;
	_keyBack        = 0;
	_keyLeft        = 0;
	_keyRight       = 0;
	_keyFire        = 0;
	_keyLight       = 0;
	_keyTowerLeft   = 0;
	_keyTowerRight  = 0;
	_keyTowerCenter = 0;
	_keyPickup      = 0;

	_lastLightsState  = true;
	_aimToMouse       = false;
	_moveToMouse      = false;
	_arcadeStyle      = false;
}

void Controller::SetProfile(const char *profile)
{
	ConfVar *p = g_conf.dm_profiles.Find(profile);
	if( p && ConfVar::typeTable == p->GetType() )
	{
		ConfControllerProfile t(p->AsTable());

		_keyForward     = GetKeyCode(t.key_forward.Get());
		_keyBack        = GetKeyCode(t.key_back.Get());
		_keyLeft        = GetKeyCode(t.key_left.Get());
		_keyRight       = GetKeyCode(t.key_right.Get());
		_keyFire        = GetKeyCode(t.key_fire.Get());
		_keyLight       = GetKeyCode(t.key_light.Get());
		_keyTowerLeft   = GetKeyCode(t.key_tower_left.Get());
		_keyTowerRight  = GetKeyCode(t.key_tower_right.Get());
		_keyTowerCenter = GetKeyCode(t.key_tower_center.Get());
		_keyPickup      = GetKeyCode(t.key_pickup.Get());

		_lastLightsState = t.lights.Get();
		_aimToMouse = t.aim_to_mouse.Get();
		_moveToMouse = t.move_to_mouse.Get();
		_arcadeStyle = t.arcade_style.Get();
	}
	else
	{
        GetConsole().Printf(1, "controller: profile '%s' not found", profile);
    }
}

void Controller::ReadControllerState(const GC_Vehicle *vehicle, VehicleState &vs)
{
	assert(vehicle);
	memset(&vs, 0, sizeof(VehicleState));

	//
	// lights
	//
	bool tmp = IsKeyPressed(_keyLight);
	if( tmp && !_lastLightKeyState && g_conf.sv_nightmode.Get() )
	{
		PLAY(SND_LightSwitch, vehicle->GetPos());
		_lastLightsState = !_lastLightsState;
	}
	_lastLightKeyState = tmp;
	vs._bLight = _lastLightsState;


	//
	// pickup
	//
	vs._bState_AllowDrop = IsKeyPressed(_keyPickup)
		|| ( IsKeyPressed(_keyForward) && IsKeyPressed(_keyBack)  )
		|| ( IsKeyPressed(_keyLeft)    && IsKeyPressed(_keyRight) );

	//
	// fire
	//
	vs._bState_Fire = IsKeyPressed(_keyFire);


	//
	// movement
	//
	if( _arcadeStyle )
	{
		vec2d tmp(0, 0);
		if( IsKeyPressed(_keyForward) ) tmp.y -= 1;
		if( IsKeyPressed(_keyBack)    ) tmp.y += 1;
		if( IsKeyPressed(_keyLeft)    ) tmp.x -= 1;
		if( IsKeyPressed(_keyRight)   ) tmp.x += 1;
		tmp.Normalize();

		bool move = tmp.x || tmp.y;
		bool sameDirection = tmp * vehicle->GetDirection() > cos(PI/4);

		bool bBack = move && !sameDirection && NULL != g_level->TraceNearest(g_level->grid_rigid_s, vehicle, 
			vehicle->GetPos(), vehicle->GetDirection() * vehicle->GetRadius());
		bool bForv = move && !bBack;

		vs._bState_MoveForward = sameDirection && bForv;
		vs._bState_MoveBack = bBack;
		vs._bExplicitBody = move;
		vs._fBodyAngle = tmp.Angle();
	}
	else
	{
		vs._bState_MoveForward = IsKeyPressed(_keyForward);
		vs._bState_MoveBack    = IsKeyPressed(_keyBack   );
		vs._bState_RotateLeft  = IsKeyPressed(_keyLeft   );
		vs._bState_RotateRight = IsKeyPressed(_keyRight  );
	}

	if( _moveToMouse )
	{
		vs._bState_Fire = vs._bState_Fire || IsMousePressed(GLFW_MOUSE_BUTTON_LEFT);
		vs._bState_AllowDrop = vs._bState_AllowDrop || IsMousePressed(GLFW_MOUSE_BUTTON_MIDDLE);

		vec2d pt;
		if( IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT) && GC_Camera::GetWorldMousePos(pt) )
		{
			vec2d tmp = pt - vehicle->GetPos() - vehicle->GetBrakingLength();
			if( tmp.sqr() > 1 )
			{
				if( tmp * vehicle->GetDirection() < 0 )
				{
					tmp = -tmp;
					vs._bState_MoveBack    = true;
				}
				else
				{
					vs._bState_MoveForward = true;
				}
				vs._bExplicitBody = true;
				vs._fBodyAngle = tmp.Angle();
			}
		}
	}



	//
	// tower control
	//
	if( _aimToMouse )
	{
		vs._bState_Fire = vs._bState_Fire || IsMousePressed(GLFW_MOUSE_BUTTON_LEFT);
		if( !_moveToMouse )
		{
			vs._bState_AllowDrop = vs._bState_AllowDrop || IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT);
		}

		vec2d pt;
		if( vehicle->GetWeapon() && GC_Camera::GetWorldMousePos(pt) )
		{
			float a = (pt - vehicle->GetPos()).Angle();
			vs._bExplicitTower = true;
			vs._fTowerAngle = a - vehicle->GetDirection().Angle() - vehicle->GetSpinup();
		}
	}
	else
	{
		vs._bState_TowerLeft   = IsKeyPressed(_keyTowerLeft);
		vs._bState_TowerRight  = IsKeyPressed(_keyTowerRight);
		vs._bState_TowerCenter = IsKeyPressed(_keyTowerCenter)
			|| (IsKeyPressed(_keyTowerLeft) && IsKeyPressed(_keyTowerRight));
		if( vs._bState_TowerCenter )
		{
			vs._bState_TowerLeft  = false;
			vs._bState_TowerRight = false;
		}
	}
}


// end of file
