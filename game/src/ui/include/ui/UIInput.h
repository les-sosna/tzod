#pragma once

#include <math/MyMath.h>

namespace UI
{
	struct IInput
	{
		virtual bool IsKeyPressed(int key) const = 0;
		virtual bool IsMousePressed(int button) const = 0;
		virtual vec2d GetMousePos() const = 0;
	};
}
