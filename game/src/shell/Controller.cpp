#include "inc/shell/detail/Controller.h"
#include "inc/shell/Config.h"
#include "KeyMapper.h"
#include <gc/VehicleState.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <ui/UIInput.h>
#include <GLFW/glfw3.h>


Controller::Controller()
  : _keyForward(0)
  , _keyBack(0)
  , _keyLeft(0)
  , _keyRight(0)
  , _keyFire(0)
  , _keyLight(0)
  , _keyTowerLeft(0)
  , _keyTowerRight(0)
  , _keyTowerCenter(0)
  , _keyPickup(0)
  , _aimToMouse(false)
  , _moveToMouse(false)
  , _arcadeStyle(false)
  , _lastLightKeyState(false)
  , _lastLightsState(true)
{
}

void Controller::SetProfile(ConfControllerProfile &profile)
{
	_keyForward     = GetKeyCode(profile.key_forward.Get());
	_keyBack        = GetKeyCode(profile.key_back.Get());
	_keyLeft        = GetKeyCode(profile.key_left.Get());
	_keyRight       = GetKeyCode(profile.key_right.Get());
	_keyFire        = GetKeyCode(profile.key_fire.Get());
	_keyLight       = GetKeyCode(profile.key_light.Get());
	_keyTowerLeft   = GetKeyCode(profile.key_tower_left.Get());
	_keyTowerRight  = GetKeyCode(profile.key_tower_right.Get());
	_keyTowerCenter = GetKeyCode(profile.key_tower_center.Get());
	_keyPickup      = GetKeyCode(profile.key_pickup.Get());

	_lastLightsState = profile.lights.Get();
	_aimToMouse = profile.aim_to_mouse.Get();
	_moveToMouse = profile.move_to_mouse.Get();
	_arcadeStyle = profile.arcade_style.Get();
}

void Controller::ReadControllerState(UI::IInput &input, World &world, const GC_Vehicle *vehicle, const vec2d *mouse, VehicleState &vs)
{
	assert(vehicle);
	memset(&vs, 0, sizeof(VehicleState));

	//
	// lights
	//
	bool keyLightPressed = input.IsKeyPressed(_keyLight);
	if (keyLightPressed && !_lastLightKeyState && world._nightMode)
	{
		_lastLightsState = !_lastLightsState;
	}
	_lastLightKeyState = keyLightPressed;
	vs._bLight = _lastLightsState;


	//
	// pickup
	//
	vs._bState_AllowDrop = input.IsKeyPressed(_keyPickup)
		|| ( input.IsKeyPressed(_keyForward) && input.IsKeyPressed(_keyBack)  )
		|| ( input.IsKeyPressed(_keyLeft)    && input.IsKeyPressed(_keyRight) );

	//
	// fire
	//
	vs._bState_Fire = input.IsKeyPressed(_keyFire);


	//
	// movement
	//
	if( _arcadeStyle )
	{
		vec2d tmp(0, 0);
		if( input.IsKeyPressed(_keyForward) ) tmp.y -= 1;
		if( input.IsKeyPressed(_keyBack)    ) tmp.y += 1;
		if( input.IsKeyPressed(_keyLeft)    ) tmp.x -= 1;
		if( input.IsKeyPressed(_keyRight)   ) tmp.x += 1;
		tmp.Normalize();

		bool move = tmp.x || tmp.y;
		bool sameDirection = tmp * vehicle->GetDirection() > cos(PI/4);

		bool bBack = move && !sameDirection && nullptr != world.TraceNearest(world.grid_rigid_s, vehicle,
			vehicle->GetPos(), vehicle->GetDirection() * vehicle->GetRadius());
		bool bForv = move && !bBack;

		vs._bState_MoveForward = sameDirection && bForv;
		vs._bState_MoveBack = bBack;
		vs._bExplicitBody = move;
		vs._fBodyAngle = tmp.Angle();
	}
	else
	{
		vs._bState_MoveForward = input.IsKeyPressed(_keyForward);
		vs._bState_MoveBack    = input.IsKeyPressed(_keyBack   );
		vs._bState_RotateLeft  = input.IsKeyPressed(_keyLeft   );
		vs._bState_RotateRight = input.IsKeyPressed(_keyRight  );
	}

	if( _moveToMouse )
	{
		vs._bState_Fire = vs._bState_Fire || input.IsMousePressed(GLFW_MOUSE_BUTTON_LEFT);
		vs._bState_AllowDrop = vs._bState_AllowDrop || input.IsMousePressed(GLFW_MOUSE_BUTTON_MIDDLE);

		if( input.IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT) && mouse )
		{
			vec2d tmp = *mouse - vehicle->GetPos() - vehicle->GetBrakingLength();
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
		vs._bState_Fire = vs._bState_Fire || input.IsMousePressed(GLFW_MOUSE_BUTTON_LEFT);
		if( !_moveToMouse )
		{
			vs._bState_AllowDrop = vs._bState_AllowDrop || input.IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT);
		}

		if( vehicle->GetWeapon() && mouse )
		{
			float a = (*mouse - vehicle->GetPos()).Angle();
			vs._bExplicitTower = true;
			vs._fTowerAngle = a - vehicle->GetDirection().Angle() - vehicle->GetSpinup();
		}
	}
	else
	{
		vs._bState_TowerLeft   = input.IsKeyPressed(_keyTowerLeft);
		vs._bState_TowerRight  = input.IsKeyPressed(_keyTowerRight);
		vs._bState_TowerCenter = input.IsKeyPressed(_keyTowerCenter)
			|| (input.IsKeyPressed(_keyTowerLeft) && input.IsKeyPressed(_keyTowerRight));
		if( vs._bState_TowerCenter )
		{
			vs._bState_TowerLeft  = false;
			vs._bState_TowerRight = false;
		}
	}
}


// end of file
