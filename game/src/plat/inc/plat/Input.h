#pragma once
#include <math/MyMath.h>

namespace Plat
{
	enum class Key;

	enum class Msg
	{
		KeyReleased,
		KeyPressed,
		PointerDown,
		PointerMove,
		PointerUp,
		PointerCancel,
		Scroll,
		ScrollPrecise,
		TAP,
	};

	enum class PointerType
	{
		Unknown,
		Mouse,
		Touch,
	};

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

	struct PointerState
	{
		PointerType type;
		vec2d position;
		bool pressed : 1;
		bool button1 : 1;
		bool button2 : 1;
		bool button3 : 1;
	};

	struct Input
	{
		virtual bool IsKeyPressed(Key key) const = 0;
		virtual PointerState GetPointerState(unsigned int index) const = 0;
		virtual GamepadState GetGamepadState(unsigned int index) const = 0;
		virtual bool GetSystemNavigationBackAvailable() const = 0;
	};
}
