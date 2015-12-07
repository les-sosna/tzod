#include "pch.h"
#include "StoreAppInput.h"

bool StoreAppInput::IsKeyPressed(UI::Key key) const
{
	return false;
}

bool StoreAppInput::IsMousePressed(int button) const
{
	return false;
}

vec2d StoreAppInput::GetMousePos() const
{
	return vec2d(0, 0);
}
