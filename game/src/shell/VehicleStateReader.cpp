#include "VehicleStateReader.h"
#include "KeyMapper.h"
#include "inc/shell/Config.h"
#include <plat/Keys.h>
#include <plat/Input.h>
#include <gc/VehicleState.h>
#include <gc/Vehicle.h>
#include <gc/Weapons.h>
#include <gc/World.h>
#include <gv/GameViewHarness.h>
#include <float.h>

VehicleStateReader::VehicleStateReader()
  : _tapFireTime(-FLT_MAX)
  , _keyForward(Plat::Key::Unknown)
  , _keyBack(Plat::Key::Unknown)
  , _keyLeft(Plat::Key::Unknown)
  , _keyRight(Plat::Key::Unknown)
  , _keyFire(Plat::Key::Unknown)
  , _keyLight(Plat::Key::Unknown)
  , _keyTowerLeft(Plat::Key::Unknown)
  , _keyTowerRight(Plat::Key::Unknown)
  , _keyTowerCenter(Plat::Key::Unknown)
  , _keyNoPickup(Plat::Key::Unknown)
  , _gamepad(-1)
  , _aimToMouse(false)
  , _moveToMouse(false)
  , _arcadeStyle(false)
  , _lastLightKeyState(false)
  , _lastLightsState(true)
{
}

void VehicleStateReader::SetProfile(ConfControllerProfile &profile)
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

	_gamepad = profile.gamepad.GetInt();
	_lastLightsState = profile.lights.Get();
	_aimToMouse = profile.aim_to_mouse.Get();
	_moveToMouse = profile.move_to_mouse.Get();
	_arcadeStyle = profile.arcade_style.Get();
}

void VehicleStateReader::ReadVehicleState(const GameViewHarness &gameViewHarness, const GC_Vehicle &vehicle, int playerIndex, Plat::Input &input, vec2d dragDirection, bool reverse, VehicleState &vs)
{
	memset(&vs, 0, sizeof(VehicleState));

	Plat::GamepadState gamepadState = _gamepad != -1 ? input.GetGamepadState(_gamepad) : Plat::GamepadState{};

	World &world = gameViewHarness.GetWorld();

	//
	// lights
	//
	bool keyLightPressed = input.IsKeyPressed(_keyLight);
	if (keyLightPressed && !_lastLightKeyState && world.GetNightMode())
	{
		_lastLightsState = !_lastLightsState;
	}
	_lastLightKeyState = keyLightPressed;
	vs.light = _lastLightsState;


	//
	// pickup
	//
	vs.pickup = !input.IsKeyPressed(_keyNoPickup)
		&& !( input.IsKeyPressed(_keyForward) && input.IsKeyPressed(_keyBack)  )
		&& !( input.IsKeyPressed(_keyLeft)    && input.IsKeyPressed(_keyRight) );

	//
	// fire
	//
	vs.attack = input.IsKeyPressed(_keyFire);
	if (!vs.attack && _tapFireTime > 0 && vehicle.GetWeapon())
	{
		vec2d dir = _tapFireTarget - vehicle.GetPos();
		if( dir.sqr() > 1 )
		{
			dir.Normalize();
			float cosDiff = Vec2dDot(dir, vehicle.GetWeapon()->GetDirection());
			AIWEAPSETTINGS ws;
			vehicle.GetWeapon()->SetupAI(&ws);
			vs.attack = cosDiff >= ws.fMaxAttackAngleCos;
		}
	}


	// move with keyboard
	if( _arcadeStyle )
	{
		vs.steering.y -= input.IsKeyPressed(_keyForward);
		vs.steering.y += input.IsKeyPressed(_keyBack);
		vs.steering.x -= input.IsKeyPressed(_keyLeft);
		vs.steering.x += input.IsKeyPressed(_keyRight);
		vs.steering.Normalize();

		vs.gas = Vec2dDot(vs.steering, vehicle.GetDirection());

		static const float threshold = std::cos(PI / 4);
		if (vs.gas > 0 && vs.gas < threshold &&
			!!world.TraceNearest(world.grid_rigid_s, &vehicle, vehicle.GetPos(), vehicle.GetDirection() * vehicle.GetRadius()))
		{
			vs.gas = -1;
		}
	}
	else
	{
		vs.gas += input.IsKeyPressed(_keyForward);
		vs.gas -= input.IsKeyPressed(_keyBack);

		if (input.IsKeyPressed(_keyLeft) || gamepadState.DPadLeft)
		{
			vs.steering = Vec2dSubDirection(vehicle.GetDirection(), Vec2dDirection(PI / 2));
		}
		else if (input.IsKeyPressed(_keyRight) || gamepadState.DPadRight)
		{
			vs.steering = Vec2dAddDirection(vehicle.GetDirection(), Vec2dDirection(PI / 2));
		}
	}

	GameViewHarness::CanvasToWorldResult c2w = {};
	Plat::PointerState pointerState = input.GetPointerState(0);
	if (pointerState.type == Plat::PointerType::Mouse)
	{
		c2w = gameViewHarness.CanvasToWorld(playerIndex, (int)pointerState.position.x, (int)pointerState.position.y);
	}

	// move with mouse
	if( _moveToMouse )
	{
		vs.attack = vs.attack || pointerState.button1;
		vs.pickup = vs.pickup || pointerState.button3;

		if( c2w.visible && pointerState.button2 )
		{
			vec2d bodyDirection = c2w.worldPos - vehicle.GetPos() - vehicle.GetBrakingLength();
			if( bodyDirection.sqr() > 10 )
			{
				bodyDirection.Normalize();
				vs.gas = Vec2dDot(bodyDirection, vehicle.GetDirection());
				if(vs.gas < 0 )
				{
					bodyDirection = -bodyDirection;
				}
				vs.steering = bodyDirection;
			}
		}
	}

	// move with tap and drag
	if (!dragDirection.IsZero())
	{
		if (reverse)
		{
			dragDirection = -dragDirection;
		}
		bool doMove = Vec2dDot(dragDirection.Norm(), vehicle.GetDirection()) > cos(PI/4);
		vs.steering = dragDirection;
		vs.gas = doMove ? (reverse ? -1.f : 1.f) : 0.f;
		if (_tapFireTime < -1.5f)
		{
			vs.weaponAngle = 0;
			vs.rotateWeapon = true;
		}
	}

	// move with gamepad
	if (gamepadState.leftThumbstickPos.sqr() > 0.01f)
	{
		vs.steering = gamepadState.leftThumbstickPos;
		vs.gas = Vec2dDot(gamepadState.leftThumbstickPos, vehicle.GetDirection());

		vec2d steeringDirection = gamepadState.leftThumbstickPos.Norm();

		static const float threshold = std::cos(PI / 3);
		if (vs.gas > 0 && Vec2dDot(steeringDirection, vehicle.GetDirection()) < threshold &&
			!!world.TraceNearest(world.grid_rigid_s, &vehicle, vehicle.GetPos(), vehicle.GetDirection() * vehicle.GetRadius()))
		{
			vs.gas = -gamepadState.leftThumbstickPos.len();
		}
	}
	if (gamepadState.leftTrigger > 0.5f)
	{
		vs.gas = 0; // break
	}


	//
	// tower control
	//
	if( _aimToMouse )
	{
		vs.attack = vs.attack || pointerState.button1;
		if( !_moveToMouse )
		{
			vs.pickup = vs.pickup || pointerState.button2;
		}

		if( vehicle.GetWeapon() && c2w.visible )
		{
			float a = (c2w.worldPos - vehicle.GetPos()).Angle();
			vs.weaponAngle = a - vehicle.GetDirection().Angle() - vehicle.GetSpinup();
			vs.rotateWeapon = true;
		}
	}
	else if(vehicle.GetWeapon())
	{
		if (input.IsKeyPressed(_keyTowerCenter) || (input.IsKeyPressed(_keyTowerLeft) && input.IsKeyPressed(_keyTowerRight)))
		{
			vs.weaponAngle = 0;
			vs.rotateWeapon = true;
		}
		else if (input.IsKeyPressed(_keyTowerLeft))
		{
			vs.weaponAngle = vehicle.GetWeapon()->GetAngleLocal() - PI / 2;
			vs.rotateWeapon = true;
		}
		else if (input.IsKeyPressed(_keyTowerRight))
		{
			vs.weaponAngle = vehicle.GetWeapon()->GetAngleLocal() + PI / 2;
			vs.rotateWeapon = true;
		}
	}

	// tower to tap
	if (_tapFireTime > 0)
	{
		float a = (_tapFireTarget - vehicle.GetPos()).Angle();
		vs.weaponAngle = a - vehicle.GetDirection().Angle() - vehicle.GetSpinup();
		vs.rotateWeapon = true;
	}

	// tower to right stick
	if (gamepadState.rightThumbstickPos.sqr() > 0.5f)
	{
		vs.weaponAngle = gamepadState.rightThumbstickPos.Angle() - vehicle.GetDirection().Angle() - vehicle.GetSpinup();
		vs.rotateWeapon = true;
	}
	else if (gamepadState.B)
	{
		vs.weaponAngle = 0;
		vs.rotateWeapon = true;
	}

	// fire with gamepad
	if (gamepadState.rightTrigger > 0.5f || gamepadState.X)
	{
		vs.attack = true;
	}
}

void VehicleStateReader::OnTap(vec2d worldPos)
{
	_tapFireTime = std::min(.25f + std::max(_tapFireTime, 0.f) * 2.f, 1.f);
	_tapFireTarget = worldPos;
}

void VehicleStateReader::Step(float dt)
{
	_tapFireTime = _tapFireTime - dt;
}
