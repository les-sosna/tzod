#include "pch.h"
#include "StoreAppWindow.h"
#include "DeviceResources.h"
#include "DisplayOrientation.h"
#include "WinStoreKeys.h"

#include <video/RenderBase.h>
#include <video/RenderD3D11.h>
#include <ui/GuiManager.h>
#include <ui/Window.h>

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

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
	const float defaultDpi = 96.0f;
	return dips * dpi / defaultDpi;
}

StoreAppWindow::StoreAppWindow(CoreWindow^ coreWindow, DX::DeviceResources &deviceResources, DX::SwapChainResources &swapChainResources)
	: _displayInformation(DisplayInformation::GetForCurrentView())
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
			float dpi = displayInformation->LogicalDpi;
			(*inputSink)->GetDesktop()->Resize(PixelsFromDips(sender->Bounds.Width, dpi), PixelsFromDips(sender->Bounds.Height, dpi));
		}
	});

	_regPointerMoved = _coreWindow->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation](CoreWindow^ sender, PointerEventArgs^ args)
	{
		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			args->Handled = (*inputSink)->ProcessMouse(
				PixelsFromDips(args->CurrentPoint->Position.X, dpi),
				PixelsFromDips(args->CurrentPoint->Position.Y, dpi), 0, UI::MSGMOUSEMOVE);
		}
	});

	_regPointerPressed = _coreWindow->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation](CoreWindow^ sender, PointerEventArgs^ args)
	{
		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			args->Handled = (*inputSink)->ProcessMouse(
				PixelsFromDips(args->CurrentPoint->Position.X, dpi),
				PixelsFromDips(args->CurrentPoint->Position.Y, dpi), 0, UI::MSGLBUTTONDOWN);
		}
	});

	_regPointerReleased = _coreWindow->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[inputSink = _inputSink, displayInformation = _displayInformation](CoreWindow^ sender, PointerEventArgs^ args)
	{
		if (*inputSink)
		{
			float dpi = displayInformation->LogicalDpi;
			args->Handled = (*inputSink)->ProcessMouse(
				PixelsFromDips(args->CurrentPoint->Position.X, dpi),
				PixelsFromDips(args->CurrentPoint->Position.Y, dpi), 0, UI::MSGLBUTTONUP);
		}
	});

	_coreWindow->PointerWheelChanged += ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(
		[inputSink = _inputSink, displayInformation = _displayInformation](CoreWindow^ sender, PointerEventArgs^ args)
	{
		if (*inputSink)
		{
			int delta = args->CurrentPoint->Properties->MouseWheelDelta;
			float dpi = displayInformation->LogicalDpi;
			args->Handled = (*inputSink)->ProcessMouse(
				PixelsFromDips(args->CurrentPoint->Position.X, dpi),
				PixelsFromDips(args->CurrentPoint->Position.Y, dpi), (float)delta / 120.f, UI::MSGMOUSEWHEEL);
		}
	});

	_regKeyDown = _coreWindow->KeyDown += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
		[inputSink = _inputSink](CoreWindow^ sender, KeyEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->ProcessKeys(UI::MSGKEYDOWN, MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey));
		}
	});

	_regKeyUp = _coreWindow->KeyUp += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
		[inputSink = _inputSink](CoreWindow^ sender, KeyEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->ProcessKeys(UI::MSGKEYUP, MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey));
		}
	});

	_regCharacterReceived = _coreWindow->CharacterReceived += ref new TypedEventHandler<CoreWindow ^, CharacterReceivedEventArgs ^>(
		[inputSink = _inputSink](CoreWindow^ sender, CharacterReceivedEventArgs^ args)
	{
		if (*inputSink)
		{
			args->Handled = (*inputSink)->ProcessText(args->KeyCode);
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

unsigned int StoreAppWindow::GetPixelWidth()
{
	float dpi = _displayInformation->LogicalDpi;
	return (unsigned int)PixelsFromDips(_coreWindow->Bounds.Width, dpi);
}

unsigned int StoreAppWindow::GetPixelHeight()
{
	float dpi = _displayInformation->LogicalDpi;
	return (unsigned int)PixelsFromDips(_coreWindow->Bounds.Height, dpi);
}

void StoreAppWindow::SetInputSink(UI::LayoutManager *inputSink)
{
	*_inputSink = inputSink;
}

