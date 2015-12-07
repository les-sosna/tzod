#include "pch.h"
#include "StoreAppInput.h"
#include "WinStoreKeys.h"
#include "DirectXHelper.h"

using namespace Windows::Graphics::Display;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::UI::Core;

StoreAppInput::StoreAppInput(CoreWindow ^coreWindow)
	: _coreWindow(coreWindow)
{
}

bool StoreAppInput::IsKeyPressed(UI::Key key) const
{
	Windows::System::VirtualKey virtualKey = UnmapWinStoreKeyCode(key);
	if (Windows::System::VirtualKey::None != virtualKey)
	{
		return (_coreWindow->GetKeyState(virtualKey) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
	}
	return false;
}

bool StoreAppInput::IsMousePressed(int button) const
{
	VirtualKey virtualKey;
	switch (button)
	{
	case 1:
		virtualKey = VirtualKey::LeftButton;
		break;
	case 2:
		virtualKey = VirtualKey::RightButton;
		break;
	case 3:
		virtualKey = VirtualKey::MiddleButton;
		break;
	default:
		return false;
	}
	return (_coreWindow->GetKeyState(virtualKey) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;;
}

vec2d StoreAppInput::GetMousePos() const
{
	Point pos = _coreWindow->PointerPosition;
	float dpi = DisplayInformation::GetForCurrentView()->LogicalDpi;
	return vec2d(DX::ConvertDipsToPixels(pos.X - _coreWindow->Bounds.Left, dpi),
		DX::ConvertDipsToPixels(pos.Y - _coreWindow->Bounds.Top, dpi));
}
