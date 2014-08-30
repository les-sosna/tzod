#pragma once

namespace UI
{
	struct IInput
	{
		virtual bool IsKeyPressed(int key) const = 0;
		virtual bool IsMousePressed(int button) const = 0;
		virtual double GetMouseX() const = 0;
		virtual double GetMouseY() const = 0;
	};
}
