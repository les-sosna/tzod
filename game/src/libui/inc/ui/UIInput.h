#pragma once
#include <math/MyMath.h>

namespace UI
{
	enum class Key;

	struct IInput
	{
		virtual bool IsKeyPressed(Key key) const = 0;
		virtual bool IsMousePressed(int button) const = 0;
		virtual vec2d GetMousePos() const = 0;
	};
}
