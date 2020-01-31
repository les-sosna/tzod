#include "pch.h"
#include "DeviceResources.h"
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
#include <video/SwapChainResources.h>

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

static const float c_defaultDpi = 96.0f;

static float PixelsFromDips(float dips, float dpi)
{
	return dips * dpi / c_defaultDpi;
}

static bool DispatchPointerMessage(Plat::AppWindow& appWindow, Plat::AppWindowInputSink& inputSink, PointerEventArgs^ args, float dpi, Plat::Msg msgHint)
{
	Plat::PointerType pointerType;
	switch (args->CurrentPoint->PointerDevice->PointerDeviceType)
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

	Plat::Msg msg;
	switch (args->CurrentPoint->Properties->PointerUpdateKind)
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

	int delta = args->CurrentPoint->Properties->MouseWheelDelta;

	vec2d pxPointerPos = { PixelsFromDips(args->CurrentPoint->Position.X, dpi),
						   PixelsFromDips(args->CurrentPoint->Position.Y, dpi) };

	return inputSink.OnPointer(appWindow, pointerType, msg, pxPointerPos, vec2d{ 0, (float)delta / 120.f }, button, args->CurrentPoint->PointerId);
}

static ComPtr<IDXGISwapChain1> CreateSwapchainForCoreWindow(ID3D11Device * d3dDevice, CoreWindow ^ coreWindow)
{
	// This sequence obtains the DXGI factory that was used to create the Direct3D device.
	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(d3dDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
	DX::ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));

	ComPtr<IDXGIAdapter> dxgiAdapter;
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));

	ComPtr<IDXGIFactory2> dxgiFactory;
	DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = 0; // todo: Match the size of the window.
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = 0;

	ComPtr<IDXGISwapChain1> swapChain;
	DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
		d3dDevice,
		reinterpret_cast<IUnknown*>(coreWindow),
		&swapChainDesc,
		nullptr,
		swapChain.ReleaseAndGetAddressOf()));

	return swapChain;
}

StoreAppWindow::StoreAppWindow(CoreWindow ^ coreWindow)
	: _gestureRecognizer(ref new GestureRecognizer())
	, _systemNavigationManager(SystemNavigationManager::GetForCurrentView())
	, _displayInformation(DisplayInformation::GetForCurrentView())
	, _coreWindow(coreWindow)
	, _cursorArrow(ref new CoreCursor(CoreCursorType::Arrow, 0))
	, _cursorIBeam(ref new CoreCursor(CoreCursorType::IBeam, 0))
	, _deviceResources()
	, _swapChainResources(CreateSwapchainForCoreWindow(_deviceResources.GetD3DDevice(), coreWindow).Get())
	, _input(coreWindow)
	, _render(new RenderD3D11(_deviceResources.GetD3DDeviceContext(), _swapChainResources))
	, _renderBinding(new RenderBinding())
{
	_regBackRequested = _systemNavigationManager->BackRequested += ref new Windows::Foundation::EventHandler<Windows::UI::Core::BackRequestedEventArgs^>(
		[self = _self, inputSink = _inputSink](Platform::Object ^ sender, BackRequestedEventArgs ^ args)
		{
			if (*inputSink)
			{
				args->Handled = (*inputSink)->OnSystemNavigationBack(**self);
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
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow ^ sender, PointerEventArgs ^ args)
		{
			gestureRecognizer->ProcessMoveEvents(args->GetIntermediatePoints());

			if (*inputSink)
			{
				args->Handled = DispatchPointerMessage(**self, **inputSink, args, displayInformation->LogicalDpi, Plat::Msg::PointerMove);
			}
		});

	_regPointerPressed = _coreWindow->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow ^ sender, PointerEventArgs ^ args)
		{
			gestureRecognizer->ProcessDownEvent(args->CurrentPoint);

			if (*inputSink)
			{
				args->Handled = DispatchPointerMessage(**self, **inputSink, args, displayInformation->LogicalDpi, Plat::Msg::PointerDown);
			}
		});

	_regPointerReleased = _coreWindow->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow ^ sender, PointerEventArgs ^ args)
		{
			gestureRecognizer->ProcessUpEvent(args->CurrentPoint);
			gestureRecognizer->CompleteGesture();

			if (*inputSink)
			{
				args->Handled = DispatchPointerMessage(**self, **inputSink, args, displayInformation->LogicalDpi, Plat::Msg::PointerUp);
			}
		});

	_coreWindow->PointerWheelChanged += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, gestureRecognizer = _gestureRecognizer](CoreWindow ^ sender, PointerEventArgs ^ args)
		{
			gestureRecognizer->ProcessMouseWheelEvent(args->CurrentPoint,
				(args->KeyModifiers & VirtualKeyModifiers::Shift) != VirtualKeyModifiers::None,
				(args->KeyModifiers & VirtualKeyModifiers::Control) != VirtualKeyModifiers::None);

			if (*inputSink)
			{
				args->Handled = DispatchPointerMessage(**self, **inputSink, args, displayInformation->LogicalDpi, Plat::Msg::Scroll);
			}
		});

	_gestureRecognizer->GestureSettings = GestureSettings::Tap;
	_gestureRecognizer->Tapped += ref new TypedEventHandler<GestureRecognizer^, TappedEventArgs^>(
		[self = _self, inputSink = _inputSink, displayInformation = _displayInformation, coreWindow = _coreWindow](GestureRecognizer ^ sender, TappedEventArgs ^ args)
		{
			if (*inputSink)
			{
				float dpi = displayInformation->LogicalDpi;
				vec2d pxPointerPosition{ PixelsFromDips(args->Position.X, dpi), PixelsFromDips(args->Position.Y, dpi) };
				unsigned int pointerID = 111; // should be unique enough :)
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

	_systemNavigationManager->BackRequested -= _regBackRequested;
}

bool StoreAppWindow::IsDeviceRemoved() const
{
	return _deviceResources.IsDeviceRemoved();
}

void StoreAppWindow::PollEvents(Plat::AppWindowInputSink & inputSink)
{
	assert(!*_inputSink && !*_self);
	*_self = this;
	*_inputSink = &inputSink;

	_coreWindow->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

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

IRender& StoreAppWindow::GetRender()
{
	HRESULT hr = _swapChainResources.SetPixelSize(_deviceResources.GetD3DDevice(), _deviceResources.GetD3DDeviceContext(), GetPixelSize());
	if (!DX::IsDeviceLost(hr))
		DX::ThrowIfFailed(hr);

	int rotationAngle = ComputeDisplayRotation(_displayInformation->NativeOrientation, _displayInformation->CurrentOrientation);
	hr = _swapChainResources.SetCurrentOrientation(_deviceResources.GetD3DDevice(), _deviceResources.GetD3DDeviceContext(), rotationAngle);
	if (!DX::IsDeviceLost(hr))
		DX::ThrowIfFailed(hr);

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

void StoreAppWindow::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = _swapChainResources.GetSwapChain()->Present(1, 0);

	if (DX::IsDeviceLost(hr))
	{
		// Do nothing - the main loop will check for device lost and recover on next tick
	}
	else if (FAILED(hr))
	{
		throw Platform::Exception::CreateException(hr);
	}
}
