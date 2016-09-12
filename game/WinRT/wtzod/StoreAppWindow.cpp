#include "pch.h"
#include "StoreAppWindow.h"
#include "DeviceResources.h"
#include "DisplayOrientation.h"
#include "WinStoreKeys.h"

#include <video/RenderBase.h>
#include <video/RenderD3D11.h>
#include <ui/GuiManager.h>
#include <ui/InputContext.h>
#include <ui/LayoutContext.h>
#include <ui/StateContext.h>
#include <ui/Window.h>

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

static ::DisplayOrientation DOFromDegrees(int degrees)
{
	switch (degrees)
	{
	default: assert(false);
	case 0: return DO_0;
	case 90: return DO_90;
	case 180: return DO_180;
	case 270: return DO_270;
	}
}

static float PixelsFromDips(float dips, float dpi)
{
	return dips * dpi / c_defaultDpi;
}

static bool DispatchPointerMessage(UI::LayoutManager &inputSink, PointerEventArgs ^args, vec2d pxWndSize, float dpi, UI::Msg msgHint)
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

	return inputSink.GetInputContext().ProcessPointer(
		inputSink.GetTextureManager(),
		inputSink.GetDesktop(),
		UI::LayoutContext(dpi / c_defaultDpi, vec2d{}, pxWndSize, true),
		UI::StateContext(),
		pxPointerPos,
		vec2d{ 0, (float)delta / 120.f },
		msg,
		button,
		pointerType,
		args->CurrentPoint->PointerId);
}

StoreAppWindow::StoreAppWindow(CoreWindow^ coreWindow, DX::DeviceResources &deviceResources, DX::SwapChainResources &swapChainResources)
	: _gestureRecognizer(ref new GestureRecognizer())
	, _displayInformation(DisplayInformation::GetForCurrentView())
	, _coreWindow(coreWindow)
	, _deviceResources(deviceResources)
	, _input(coreWindow)
	, _render(RenderCreateD3D11(deviceResources.GetD3DDeviceContext(), nullptr/*swapChainResources.GetBackBufferRenderTargetView()*/))
	, _inputSink(std::make_shared<UI::LayoutManager*>())
{
	_render->SetDisplayOrientation(DOFromDegrees(ComputeDisplayRotation(_displayInformation->NativeOrientation, _displayInformation->CurrentOrientation)));
	_regOrientationChanged = _displayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(
		[this](DisplayInformation^ sender, Platform::Object^)
	{
		_render->SetDisplayOrientation(DOFromDegrees(ComputeDisplayRotation(sender->NativeOrientation, sender->CurrentOrientation)));
	});

	_regSizeChanged = _coreWindow->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation](CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
	{
		if (*inputSink)
		{
			(*inputSink)->GetDesktop()->Resize(sender->Bounds.Width, sender->Bounds.Height);
		}
	});

	_regPointerMoved = _coreWindow->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessMoveEvents(args->GetIntermediatePoints());

		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			vec2d pxSize{ PixelsFromDips(sender->Bounds.Width, dpi), PixelsFromDips(sender->Bounds.Height, dpi) };
			args->Handled = DispatchPointerMessage(**inputSink, args, pxSize, dpi, UI::Msg::PointerMove);
		}
	});

	_regPointerPressed = _coreWindow->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessDownEvent(args->CurrentPoint);

		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			vec2d pxSize{ PixelsFromDips(sender->Bounds.Width, dpi), PixelsFromDips(sender->Bounds.Height, dpi) };
			args->Handled = DispatchPointerMessage(**inputSink, args, pxSize, dpi, UI::Msg::PointerDown);
		}
	});

	_regPointerReleased = _coreWindow->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow^ sender, PointerEventArgs^ args)
	{
		gestureRecognizer->ProcessUpEvent(args->CurrentPoint);
		gestureRecognizer->CompleteGesture();

		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			vec2d pxSize{ PixelsFromDips(sender->Bounds.Width, dpi), PixelsFromDips(sender->Bounds.Height, dpi) };
			args->Handled = DispatchPointerMessage(**inputSink, args, pxSize, dpi, UI::Msg::PointerUp);
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
			float dpi = displayInformation->LogicalDpi;
			vec2d pxSize{ PixelsFromDips(sender->Bounds.Width, dpi), PixelsFromDips(sender->Bounds.Height, dpi) };
			args->Handled = DispatchPointerMessage(**inputSink, args, pxSize, dpi, UI::Msg::Scroll);
		}
	});

	_gestureRecognizer->GestureSettings = GestureSettings::Tap;
	_gestureRecognizer->Tapped += ref new TypedEventHandler<GestureRecognizer ^, TappedEventArgs ^>(
		[inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](GestureRecognizer ^sender, TappedEventArgs ^args)
	{
		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			vec2d pxWndSize{ PixelsFromDips(coreWindow->Bounds.Width, dpi), PixelsFromDips(coreWindow->Bounds.Height, dpi) };
			vec2d pxPointerPosition{ PixelsFromDips(args->Position.X, dpi), PixelsFromDips(args->Position.Y, dpi) };

			unsigned int pointerID = 111; // should be unique enough :)
			(*inputSink)->GetInputContext().ProcessPointer(
				(*inputSink)->GetTextureManager(),
				(*inputSink)->GetDesktop(),
				UI::LayoutContext(dpi / c_defaultDpi, vec2d{}, pxWndSize, true),
				UI::StateContext(),
				pxPointerPosition,
				vec2d{}, // scroll offset
				UI::Msg::TAP,
				1,
				UI::PointerType::Touch,
				pointerID);
		}
	});

	_regKeyDown = _coreWindow->KeyDown += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
		[inputSink = _inputSink](CoreWindow^ sender, KeyEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->GetInputContext().ProcessKeys(
				(*inputSink)->GetDesktop(),
				UI::Msg::KEYDOWN,
				MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey));
		}
	});

	_regKeyUp = _coreWindow->KeyUp += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
		[inputSink = _inputSink](CoreWindow^ sender, KeyEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->GetInputContext().ProcessKeys(
				(*inputSink)->GetDesktop(),
				UI::Msg::KEYUP,
				MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey));
		}
	});

	_regCharacterReceived = _coreWindow->CharacterReceived += ref new TypedEventHandler<CoreWindow ^, CharacterReceivedEventArgs ^>(
		[inputSink = _inputSink](CoreWindow^ sender, CharacterReceivedEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->GetInputContext().ProcessText((*inputSink)->GetDesktop(), args->KeyCode);
		}
	});
}

StoreAppWindow::~StoreAppWindow()
{
	_coreWindow->CharacterReceived -= _regCharacterReceived;
	_coreWindow->KeyUp -= _regKeyUp;
	_coreWindow->KeyDown -= _regKeyDown;
	_coreWindow->PointerReleased -= _regPointerReleased;
	_coreWindow->PointerPressed -= _regPointerMoved;
	_coreWindow->PointerMoved -= _regPointerMoved;
	_coreWindow->SizeChanged -= _regSizeChanged;

	_displayInformation->OrientationChanged -= _regOrientationChanged;

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

float StoreAppWindow::GetPixelWidth() const
{
	return PixelsFromDips(_coreWindow->Bounds.Width, _displayInformation->LogicalDpi);
}

float StoreAppWindow::GetPixelHeight() const
{
	return PixelsFromDips(_coreWindow->Bounds.Height, _displayInformation->LogicalDpi);
}

float StoreAppWindow::GetLayoutScale() const
{
	return _displayInformation->LogicalDpi / c_defaultDpi;
}

void StoreAppWindow::SetInputSink(UI::LayoutManager *inputSink)
{
	*_inputSink = inputSink;

	if (inputSink)
	{
		float width = GetPixelWidth() / GetLayoutScale();
		float height = GetPixelHeight() / GetLayoutScale();
		inputSink->GetDesktop()->Resize(width, height);
	}
}

