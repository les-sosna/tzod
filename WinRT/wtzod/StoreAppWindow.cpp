#include "pch.h"
#include "DirectXHelper.h"
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
#include <video/RenderBinding.h>
#include <video/RenderD3D11.h>

using namespace Microsoft::WRL;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::Devices::Input;

static bool DispatchPointerMessage(Plat::AppWindow& appWindow, Plat::AppWindowInputSink& inputSink, PointerEventArgs^ args, Plat::Msg msgHint, double scale)
{
	Plat::PointerType pointerType;
	auto currentPoint = args->CurrentPoint;
	switch (currentPoint->PointerDevice->PointerDeviceType)
	{
	case PointerDeviceType::Mouse:
	case PointerDeviceType::Pen:
		pointerType = Plat::PointerType::Mouse;
		break;

	case PointerDeviceType::Touch:
		pointerType = Plat::PointerType::Touch;
		break;

	default:
		pointerType = Plat::PointerType::Unknown;
		break;
	}

	int button;
	auto currentPointProperties = currentPoint->Properties;
	switch (currentPointProperties->PointerUpdateKind)
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

	Plat::Msg msg;
	switch (currentPointProperties->PointerUpdateKind)
	{
	case PointerUpdateKind::LeftButtonPressed:
	case PointerUpdateKind::RightButtonPressed:
	case PointerUpdateKind::MiddleButtonPressed:
		msg = Plat::Msg::PointerDown;
		break;

	case PointerUpdateKind::LeftButtonReleased:
	case PointerUpdateKind::RightButtonReleased:
	case PointerUpdateKind::MiddleButtonReleased:
		msg = Plat::Msg::PointerUp;
		break;

	default:
		switch (msgHint)
		{
		case Plat::Msg::Scroll:
		case Plat::Msg::PointerMove:
			msg = msgHint;
			break;
		default:
			return false; // unknown button or something else we do not handle
		}
		break;
	}

	int delta = currentPointProperties->MouseWheelDelta;
	auto pos = currentPoint->Position;
	vec2d pxPointerPos = { float(pos.X * scale), float(pos.Y * scale) };
	return inputSink.OnPointer(appWindow, pointerType, msg, pxPointerPos, vec2d{ 0, (float)delta / 120.f }, button, currentPoint->PointerId);
}

StoreAppWindow::StoreAppWindow(CoreWindow ^ coreWindow)
	: _gestureRecognizer(ref new GestureRecognizer())
	, _systemNavigationManager(SystemNavigationManager::GetForCurrentView())
	, _displayInformation(DisplayInformation::GetForCurrentView())
	, _coreWindow(coreWindow)
	, _cursorArrow(ref new CoreCursor(CoreCursorType::Arrow, 0))
	, _cursorIBeam(ref new CoreCursor(CoreCursorType::IBeam, 0))
	, _input(coreWindow, _displayInformation->RawPixelsPerViewPixel)
	, _displayRotation(ComputeDisplayRotation(_displayInformation->NativeOrientation, _displayInformation->CurrentOrientation))
{
	_regBackRequested = _systemNavigationManager->BackRequested += ref new Windows::Foundation::EventHandler<Windows::UI::Core::BackRequestedEventArgs^>(
		[self = _self, inputSink = _inputSink](Platform::Object ^ sender, BackRequestedEventArgs ^ args)
		{
			if (*inputSink)
			{
				args->Handled = (*inputSink)->OnSystemNavigationBack(**self);
			}
		});

	_regDpiChanged = _displayInformation->DpiChanged += ref new Windows::Foundation::TypedEventHandler<Windows::Graphics::Display::DisplayInformation^, Platform::Object^>(
		[self = _self, inputSink = _inputSink](Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
		{
			if (*self)
			{
				(*self)->_input.SetScale(sender->RawPixelsPerViewPixel);
			}
		});

	_regOrientationChanged = _displayInformation->OrientationChanged += ref new Windows::Foundation::TypedEventHandler<Windows::Graphics::Display::DisplayInformation^, Platform::Object^>(
		[self = _self, inputSink = _inputSink](Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
		{
			if (*self)
			{
				(*self)->_displayRotation = ComputeDisplayRotation(sender->NativeOrientation, sender->CurrentOrientation);
			}
		});

	_regVisibilityChanged = coreWindow->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(
		[self = _self](CoreWindow ^ sender, VisibilityChangedEventArgs ^ args)
		{
			if (*self)
			{
				(*self)->_visible = args->Visible;
				args->Handled = true;
			}
		});

	_regClosed = coreWindow->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(
		[self = _self](CoreWindow ^ sender, CoreWindowEventArgs ^ args)
		{
			if (*self)
			{
				(*self)->_shouldClose = true;
				args->Handled = true;
			}
		});

	_regPointerMoved = _coreWindow->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[self = _self, inputSink = _inputSink, gestureRecognizer = _gestureRecognizer](CoreWindow ^ sender, PointerEventArgs ^ args)
		{
			gestureRecognizer->ProcessMoveEvents(args->GetIntermediatePoints());

			if (*inputSink && *self)
			{
				args->Handled = DispatchPointerMessage(**self, **inputSink, args, Plat::Msg::PointerMove, (*self)->_input.GetScale());
			}
		});

	_regPointerPressed = _coreWindow->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow ^ sender, PointerEventArgs ^ args)
		{
			gestureRecognizer->ProcessDownEvent(args->CurrentPoint);

			if (*inputSink)
			{
				args->Handled = DispatchPointerMessage(**self, **inputSink, args, Plat::Msg::PointerDown, (*self)->_input.GetScale());
			}
		});

	_regPointerReleased = _coreWindow->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[self = _self, inputSink = _inputSink, gestureRecognizer = _gestureRecognizer](CoreWindow ^ sender, PointerEventArgs ^ args)
		{
			gestureRecognizer->ProcessUpEvent(args->CurrentPoint);
			gestureRecognizer->CompleteGesture();

			if (*inputSink)
			{
				args->Handled = DispatchPointerMessage(**self, **inputSink, args, Plat::Msg::PointerUp, (*self)->_input.GetScale());
			}
		});

	_coreWindow->PointerWheelChanged += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[self = _self, inputSink = _inputSink, gestureRecognizer = _gestureRecognizer](CoreWindow ^ sender, PointerEventArgs ^ args)
		{
			gestureRecognizer->ProcessMouseWheelEvent(args->CurrentPoint,
				(args->KeyModifiers & VirtualKeyModifiers::Shift) != VirtualKeyModifiers::None,
				(args->KeyModifiers & VirtualKeyModifiers::Control) != VirtualKeyModifiers::None);

			if (*inputSink)
			{
				args->Handled = DispatchPointerMessage(**self, **inputSink, args, Plat::Msg::Scroll, (*self)->_input.GetScale());
			}
		});

	_gestureRecognizer->GestureSettings = GestureSettings::Tap;
	_gestureRecognizer->Tapped += ref new TypedEventHandler<GestureRecognizer^, TappedEventArgs^>(
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](GestureRecognizer ^ sender, TappedEventArgs ^ args)
		{
			if (*inputSink)
			{
				auto scale = (*self)->_input.GetScale();
				vec2d pxPointerPosition{ float(args->Position.X * scale), float(args->Position.Y * scale) };
				unsigned int pointerID = 111; // should be unique enough
				(*inputSink)->OnPointer(**self, Plat::PointerType::Touch, Plat::Msg::TAP, pxPointerPosition, vec2d{}/*offset*/, 1/*buttons*/, pointerID);
			}
		});

	_regKeyDown = _coreWindow->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](CoreWindow ^ sender, KeyEventArgs ^ args)
		{
			if (*inputSink)
			{
				args->Handled = (*inputSink)->OnKey(**self, MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey), Plat::Msg::KeyPressed);
			}
		});

	_regKeyUp = _coreWindow->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](CoreWindow ^ sender, KeyEventArgs ^ args)
		{
			if (*inputSink)
			{
				args->Handled = (*inputSink)->OnKey(**self, MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey), Plat::Msg::KeyReleased);
			}
		});

	_regCharacterReceived = _coreWindow->CharacterReceived += ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](CoreWindow ^ sender, CharacterReceivedEventArgs ^ args)
		{
			if (*inputSink)
			{
				args->Handled = (*inputSink)->OnChar(**self, args->KeyCode);
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
	_coreWindow->Closed -= _regClosed;
	_coreWindow->VisibilityChanged -= _regVisibilityChanged;
	_displayInformation->OrientationChanged -= _regOrientationChanged;
	_displayInformation->DpiChanged -= _regDpiChanged;
	_systemNavigationManager->BackRequested -= _regBackRequested;
}

void StoreAppWindow::PollEvents(Plat::AppWindowInputSink & inputSink, CoreProcessEventsOption options)
{
	assert(!*_inputSink && !*_self);
	*_self = this;
	*_inputSink = &inputSink;

	_coreWindow->Dispatcher->ProcessEvents(options);

	// Remove the sink so that handlers could no-op.
	*_inputSink = nullptr;
	*_self = nullptr;
}

Plat::Clipboard& StoreAppWindow::GetClipboard()
{
	return _clipboard;
}

Plat::Input& StoreAppWindow::GetInput()
{
	return _input;
}

vec2d StoreAppWindow::GetPixelSize() const
{
	auto coreWindowBounds = _coreWindow->Bounds;
	return vec2d{ float(coreWindowBounds.Width * _input.GetScale()), float(coreWindowBounds.Height * _input.GetScale()) };
}

float StoreAppWindow::GetLayoutScale() const
{
	return (float) _input.GetScale();
}

void StoreAppWindow::SetCanNavigateBack(bool canNavigateBack)
{
	_systemNavigationManager->AppViewBackButtonVisibility =
		canNavigateBack ? AppViewBackButtonVisibility::Visible : AppViewBackButtonVisibility::Collapsed;
}

void StoreAppWindow::SetMouseCursor(Plat::MouseCursor mouseCursor)
{
	if (_mouseCursor != mouseCursor)
	{
		_mouseCursor = mouseCursor;
		switch (mouseCursor)
		{
		case Plat::MouseCursor::Arrow:
			_coreWindow->PointerCursor = _cursorArrow;
			break;
		case Plat::MouseCursor::IBeam:
			_coreWindow->PointerCursor = _cursorIBeam;
			break;
		default:
			_coreWindow->PointerCursor = nullptr;
			break;
		}
	}
}
