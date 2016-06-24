#include "Controller.h"
#include "KeyMapper.h"
#include "inc/shell/Config.h"
#include <gc/VehicleState.h>
#include <gc/Vehicle.h>
#include <gc/Weapons.h>
#include <gc/World.h>
#include <ui/Keys.h>
#include <ui/UIInput.h>
#include <float.h>

Controller::Controller()
  : _tapFireTime(-FLT_MAX)
  , _keyForward(UI::Key::Unknown)
  , _keyBack(UI::Key::Unknown)
  , _keyLeft(UI::Key::Unknown)
  , _keyRight(UI::Key::Unknown)
  , _keyFire(UI::Key::Unknown)
  , _keyLight(UI::Key::Unknown)
  , _keyTowerLeft(UI::Key::Unknown)
  , _keyTowerRight(UI::Key::Unknown)
  , _keyTowerCenter(UI::Key::Unknown)
  , _keyNoPickup(UI::Key::Unknown)
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
	_keyNoPickup    = GetKeyCode(profile.key_no_pickup.Get());

	_lastLightsState = profile.lights.Get();
	_aimToMouse = profile.aim_to_mouse.Get();
	_moveToMouse = profile.move_to_mouse.Get();
	_arcadeStyle = profile.arcade_style.Get();
}

void Controller::ReadControllerState(UI::IInput &input, World &world, const GC_Vehicle &vehicle, const vec2d *mouse, vec2d dragDirection, bool reverse, VehicleState &vs)
{
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
	vs._bState_AllowDrop = !input.IsKeyPressed(_keyNoPickup)
		&& !( input.IsKeyPressed(_keyForward) && input.IsKeyPressed(_keyBack)  )
		&& !( input.IsKeyPressed(_keyLeft)    && input.IsKeyPressed(_keyRight) );

	//
	// fire
	//
	vs._bState_Fire = input.IsKeyPressed(_keyFire);
    if (_tapFireTime > 0 && !vs._bState_Fire && vehicle.GetWeapon())
    {
        vec2d dir = _tapFireTarget - vehicle.GetPos();
        if( dir.sqr() > 1 )
        {
            dir.Normalize();
            float cosDiff = Vec2dDot(dir, vehicle.GetWeapon()->GetDirection());
            AIWEAPSETTINGS ws;
            vehicle.GetWeapon()->SetupAI(&ws);
            vs._bState_Fire = cosDiff >= ws.fMaxAttackAngleCos;
        }
    }


	// move with keyboard
	if( _arcadeStyle )
	{
		vec2d tmp{0, 0};
		if( input.IsKeyPressed(_keyForward) ) tmp.y -= 1;
		if( input.IsKeyPressed(_keyBack)    ) tmp.y += 1;
		if( input.IsKeyPressed(_keyLeft)    ) tmp.x -= 1;
		if( input.IsKeyPressed(_keyRight)   ) tmp.x += 1;
		tmp.Normalize();

		bool move = !tmp.IsZero();
		bool sameDirection = Vec2dDot(tmp, vehicle.GetDirection()) > cos(PI/4);

		bool bBack = move && !sameDirection && nullptr != world.TraceNearest(world.grid_rigid_s, &vehicle,
			vehicle.GetPos(), vehicle.GetDirection() * vehicle.GetRadius());
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

	// move with mouse
	if( _moveToMouse )
	{
		vs._bState_Fire = vs._bState_Fire || input.IsMousePressed(1);
		vs._bState_AllowDrop = vs._bState_AllowDrop || input.IsMousePressed(3);

		if( input.IsMousePressed(2) && mouse )
		{
			vec2d tmp = *mouse - vehicle.GetPos() - vehicle.GetBrakingLength();
			if( tmp.sqr() > 1 )
			{
				if(Vec2dDot(tmp, vehicle.GetDirection()) < 0 )
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


	// move with tap and drag
	if (!dragDirection.IsZero())
	{
		vs._bExplicitBody = dragDirection.len() > 20;
		if (vs._bExplicitBody)
		{
			if (reverse)
			{
				dragDirection = -dragDirection;
			}
			bool doMove = Vec2dDot(dragDirection.Norm(), vehicle.GetDirection()) > cos(PI/4);
			vs._fBodyAngle = dragDirection.Angle();
			vs._bState_MoveForward = !reverse && doMove;
			vs._bState_MoveBack = reverse && doMove;

			if (_tapFireTime < -1.5f)
			{
				vs._bState_TowerCenter = true;
			}
		}
	}


	//
	// tower control
	//
	if( _aimToMouse )
	{
		vs._bState_Fire = vs._bState_Fire || input.IsMousePressed(1);
		if( !_moveToMouse )
		{
			vs._bState_AllowDrop = vs._bState_AllowDrop || input.IsMousePressed(2);
		}

		if( vehicle.GetWeapon() && mouse )
		{
			float a = (*mouse - vehicle.GetPos()).Angle();
			vs._bExplicitTower = true;
			vs._fTowerAngle = a - vehicle.GetDirection().Angle() - vehicle.GetSpinup();
		}
	}
	else
	{
		vs._bState_TowerLeft |= input.IsKeyPressed(_keyTowerLeft);
		vs._bState_TowerRight |= input.IsKeyPressed(_keyTowerRight);
		vs._bState_TowerCenter |= input.IsKeyPressed(_keyTowerCenter)
			|| (input.IsKeyPressed(_keyTowerLeft) && input.IsKeyPressed(_keyTowerRight));
	}

	// tower to tap
	if (_tapFireTime > 0)
	{
		float a = (_tapFireTarget - vehicle.GetPos()).Angle();
		vs._bExplicitTower = true;
		vs._fTowerAngle = a - vehicle.GetDirection().Angle() - vehicle.GetSpinup();
	}


	if (!vs._bExplicitTower && vs._bState_TowerCenter)
	{
		vs._bState_TowerLeft = false;
		vs._bState_TowerRight = false;
	}
}

void Controller::OnTap(vec2d worldPos)
{
	_tapFireTime = std::min(.25f + std::max(_tapFireTime, 0.f) * 2.f, 1.f);
	_tapFireTarget = worldPos;
}

void Controller::Step(float dt)
{
	_tapFireTime = _tapFireTime - dt;
}
