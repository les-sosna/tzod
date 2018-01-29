#include "pch.h"
#include "DeviceResources.h"
#include "DisplayOrientation.h"
#include "StoreAppWindow.h"
#include "WinStoreKeys.h"

#include <ui/DataContext.h>
#include <ui/GuiManager.h>
#include <ui/InputContext.h>
#include <ui/LayoutContext.h>
#include <ui/StateContext.h>
#include <ui/Window.h>

#include <video/RenderBase.h>
#include <video/RenderD3D11.h>
#include <video/SwapChainResources.h>

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::Devices::Input;

static const float c_defaultDpi = 96.0f;

static float PixelsFromDips(float dips, float dpi)
{
	return dips * dpi / c_defaultDpi;
}

static bool DispatchPointerMessage(AppWindowInputSink &inputSink, PointerEventArgs ^args, float dpi, UI::Msg msgHint)
{
	UI::PointerType pointerType;
	switch (args->CurrentPoint->PointerDevice->PointerDeviceType)
	{
	case PointerDeviceType::Mouse:
	case PointerDeviceType::Pen:
		pointerType = UI::PointerType::Mouse;
		break;

	case PointerDeviceType::Touch:
		pointerType = UI::PointerType::Touch;
		break;

	default:
		pointerType = UI::PointerType::Unknown;
		break;
	}

	int button;
	switch (args->CurrentPoint->Properties->PointerUpdateKind)
	{
	case PointerUpdateKind::LeftButtonPressed:
	case PointerUpdateKind::LeftButtonReleased:
		button = 1;
		break;

	case PointerUpdateKind::RightButtonPressed:
	case PointerUpdateKind::RightButtonReleased:
		button = 2;
		break;

	case PointerUpdateKind::MiddleButtonPressed:
	case PointerUpdateKind::MiddleButtonReleased:
		button = 3;
		break;

	default:
		button = 0;
		break;
	}

	UI::Msg msg;
	switch (args->CurrentPoint->Properties->PointerUpdateKind)
	{
	case PointerUpdateKind::LeftButtonPressed:
	case PointerUpdateKind::RightButtonPressed:
	case PointerUpdateKind::MiddleButtonPressed:
		msg = UI::Msg::PointerDown;
		break;

	case PointerUpdateKind::LeftButtonReleased:
	case PointerUpdateKind::RightButtonReleased:
	case PointerUpdateKind::MiddleButtonReleased:
		msg = UI::Msg::PointerUp;
		break;

	default:
		switch (msgHint)
		{
		case UI::Msg::Scroll:
		case UI::Msg::PointerMove:
			msg = msgHint;
			break;
		default:
			return false; // unknown button or something else we do not handle
		}
		break;
	}

	int delta = args->CurrentPoint->Properties->MouseWheelDelta;
	
	vec2d pxPointerPos = { PixelsFromDips(args->CurrentPoint->Position.X, dpi),
	                       PixelsFromDips(args->CurrentPoint->Position.Y, dpi) };

	return inputSink.OnPointer(pointerType, msg, pxPointerPos, vec2d{ 0, (float)delta / 120.f }, button, args->CurrentPoint->PointerId);
}

StoreAppWindow::StoreAppWindow(CoreWindow^ coreWindow, DX::DeviceResources &deviceResources, SwapChainResources &swapChainResources)
	: _gestureRecognizer(ref new GestureRecognizer())
	, _systemNavigationManager(SystemNavigationManager::GetForCurrentView())
	, _displayInformation(DisplayInformation::GetForCurrentView())
	, _coreWindow(coreWindow)
	, _cursorArrow(ref new CoreCursor(CoreCursorType::Arrow, 0))
	, _cursorIBeam(ref new CoreCursor(CoreCursorType::IBeam, 0))
	, _deviceResources(deviceResources)
	, _swapChainResources(swapChainResources)
	, _input(coreWindow)
	, _render(new RenderD3D11(deviceResources.GetD3DDeviceContext(), swapChainResources))
{
	_regBackRequested = _systemNavigationManager->BackRequested += ref new Windows::Foundation::EventHandler<Windows::UI::Core::BackRequestedEventArgs ^>(
		[inputSink = _inputSink](Platform::Object ^sender, BackRequestedEventArgs ^args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->OnSystemNavigationBack();
		}
	});

	_regPointerMoved = _coreWindow->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessMoveEvents(args->GetIntermediatePoints());

		if (*inputSink)
		{
			args->Handled = DispatchPointerMessage(**inputSink, args, displayInformation->LogicalDpi, UI::Msg::PointerMove);
		}
	});

	_regPointerPressed = _coreWindow->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessDownEvent(args->CurrentPoint);

		if (*inputSink)
		{
			args->Handled = DispatchPointerMessage(**inputSink, args, displayInformation->LogicalDpi, UI::Msg::PointerDown);
		}
	});

	_regPointerReleased = _coreWindow->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessUpEvent(args->CurrentPoint);
		gestureRecognizer->CompleteGesture();

		if (*inputSink)
		{
			args->Handled = DispatchPointerMessage(**inputSink, args, displayInformation->LogicalDpi, UI::Msg::PointerUp);
		}
	});

	_coreWindow->PointerWheelChanged += ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessMouseWheelEvent(args->CurrentPoint,
			(args->KeyModifiers & VirtualKeyModifiers::Shift) != VirtualKeyModifiers::None,
			(args->KeyModifiers & VirtualKeyModifiers::Control) != VirtualKeyModifiers::None);

		if (*inputSink)
		{
			args->Handled = DispatchPointerMessage(**inputSink, args, displayInformation->LogicalDpi, UI::Msg::Scroll);
		}
	});

	_gestureRecognizer->GestureSettings = GestureSettings::Tap;
	_gestureRecognizer->Tapped += ref new TypedEventHandler<GestureRecognizer ^, TappedEventArgs ^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](GestureRecognizer ^sender, TappedEventArgs ^args)
	{
		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			vec2d pxPointerPosition{ PixelsFromDips(args->Position.X, dpi), PixelsFromDips(args->Position.Y, dpi) };
			unsigned int pointerID = 111; // should be unique enough :)
			(*inputSink)->OnPointer(UI::PointerType::Touch, UI::Msg::TAP, pxPointerPosition, vec2d{}/*offset*/, 1/*buttons*/, pointerID);
		}
	});

	_regKeyDown = _coreWindow->KeyDown += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](CoreWindow^ sender, KeyEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->OnKey(MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey), UI::Msg::KeyPressed);
		}
	});

	_regKeyUp = _coreWindow->KeyUp += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](CoreWindow^ sender, KeyEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->OnKey(MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey), UI::Msg::KeyReleased);
		}
	});

	_regCharacterReceived = _coreWindow->CharacterReceived += ref new TypedEventHandler<CoreWindow ^, CharacterReceivedEventArgs ^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](CoreWindow^ sender, CharacterReceivedEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->OnChar(args->KeyCode);
		}
	});
}

StoreAppWindow::~StoreAppWindow()
{
	_coreWindow->CharacterReceived -= _regCharacterReceived;
	_coreWindow->KeyUp -= _regKeyUp;
	_coreWindow->KeyDown -= _regKeyDown;
	_coreWindow->PointerReleased -= _regPointerReleased;
	_coreWindow->PointerPressed -= _regPointerPressed;
	_coreWindow->PointerMoved -= _regPointerMoved;

	_systemNavigationManager->BackRequested -= _regBackRequested;

	// Events may still fire after the event handler is unregistered.
	// Remove the sink so that handlers could no-op.
	*_inputSink = nullptr;
}

UI::IClipboard& StoreAppWindow::GetClipboard()
{
	return _clipboard;
}

UI::IInput& StoreAppWindow::GetInput()
{
	return _input;
}

IRender& StoreAppWindow::GetRender()
{
	return *_render;
}

int StoreAppWindow::GetDisplayRotation() const
{
	return ComputeDisplayRotation(_displayInformation->NativeOrientation, _displayInformation->CurrentOrientation);
}

vec2d StoreAppWindow::GetPixelSize() const
{
	auto coreWindowBounds = _coreWindow->Bounds;
	float dpi = _displayInformation->LogicalDpi;
	return vec2d{ PixelsFromDips(coreWindowBounds.Width, dpi), PixelsFromDips(coreWindowBounds.Height, dpi) };
}

float StoreAppWindow::GetLayoutScale() const
{
	return _displayInformation->LogicalDpi / c_defaultDpi;
}

void StoreAppWindow::SetCanNavigateBack(bool canNavigateBack)
{
	_systemNavigationManager->AppViewBackButtonVisibility =
		canNavigateBack ? AppViewBackButtonVisibility::Visible : AppViewBackButtonVisibility::Collapsed;
}

void StoreAppWindow::SetMouseCursor(MouseCursor mouseCursor)
{
	if (_mouseCursor != mouseCursor)
	{
		_mouseCursor = mouseCursor;
		switch (mouseCursor)
		{
		case MouseCursor::Arrow:
			_coreWindow->PointerCursor = _cursorArrow;
			break;
		case MouseCursor::IBeam:
			_coreWindow->PointerCursor = _cursorIBeam;
			break;
		default:
			_coreWindow->PointerCursor = nullptr;
			break;
		}
	}
}

void StoreAppWindow::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = _swapChainResources.GetSwapChain()->Present(1, 0);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		// Do nothing - the main loop will check for device lost and recover on next tick
	}
	else if (FAILED(hr))
	{
		throw Platform::Exception::CreateException(hr);
	}
}
