#pragma once
#include <math/MyMath.h>

namespace UI
{
	enum class Key;

	struct GamepadState
	{
		float leftTrigger;
		float rightTrigger;
		vec2d leftThumbstickPos;
		vec2d rightThumbstickPos;
		bool A : 1;
		bool B : 1;
		bool X : 1;
		bool Y : 1;
		bool leftShoulder : 1;
		bool rightShoulder : 1;
		bool view : 1;
		bool menu : 1;
		bool leftThumbstickPressed : 1;
		bool rightThumbstickPressed : 1;
		bool DPadUp : 1;
		bool DPadRight : 1;
		bool DPadDown : 1;
		bool DPadLeft : 1;
	};

	struct IInput
	{
		virtual bool IsKeyPressed(Key key) const = 0;
		virtual bool IsMousePressed(int button) const = 0;
		virtual vec2d GetMousePos() const = 0;
		virtual GamepadState GetGamepadState(unsigned int index) const = 0;
	};
}
