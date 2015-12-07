#include "pch.h"
#include "FrameworkView.h"
#include "DeviceResources.h"
#include "SwapChainResources.h"
#include "DisplayOrientation.h"

// todo: move to main
#include <app/View.h>
#include <fs/FileSystem.h>

// todo: move to plat-winstore
#include <app/AppWindow.h>
#include <ui/Clipboard.h>
#include <ui/UIInput.h>
#include <ui/GuiManager.h>
#include <ui/Window.h>
#include <video/RenderBase.h>
#include <video/RenderD3D11.h>

using namespace wtzod;

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

FrameworkView::FrameworkView()
	: m_windowClosed(false)
	, m_windowVisible(true)
	, _fs(FS::CreateOSFileSystem("StoreData/data"))
	, _logger(100, 500)
	, _app(*_fs, _logger)
{
	// todo: move out of view class
	CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &FrameworkView::OnAppSuspending);
	CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &FrameworkView::OnAppResuming);
}

FrameworkView::~FrameworkView()
{
}

// The first method called when the IFrameworkView is being created.
void FrameworkView::Initialize(CoreApplicationView^ coreApplicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	coreApplicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &FrameworkView::OnAppViewActivated);

	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	assert(!m_deviceResources);
	m_deviceResources = std::make_shared<DX::DeviceResources>();
}

#include <ui/Keys.h>
UI::Key MapWinStoreKeyCode(Windows::System::VirtualKey platformKey, bool isExtended)
{
	if (platformKey == Windows::System::VirtualKey::Enter && isExtended)
	{
		return UI::Key::NumEnter;
	}

	switch (platformKey)
	{
#define GEN_KEY_ENTRY(platformKey, uiKey) case platformKey: return uiKey;
#include "WinStoreKeys.h"
#undef GEN_KEY_ENTRY
	default:
		break;
	}

	return UI::Key::Unknown;
}


class StoreAppClipboard : public UI::IClipboard
{
public:
	const char* GetClipboardText() const override
	{
		return "hello";
	}
	void SetClipboardText(std::string text) override
	{
	}
};

class StoreAppInput : public UI::IInput
{
public:
	bool IsKeyPressed(UI::Key key) const override { return false; }
	bool IsMousePressed(int button) const override { return false; }
	vec2d GetMousePos() const override { return vec2d(0, 0); }
};

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

class StoreAppWindow : public AppWindow
{
public:
	StoreAppWindow(CoreWindow^ coreWindow, DX::DeviceResources &deviceResources, DX::SwapChainResources &swapChainResources)
		: _displayInformation(DisplayInformation::GetForCurrentView())
		, _coreWindow(coreWindow)
		, _deviceResources(deviceResources)
		, _render(RenderCreateD3D11(deviceResources.GetD3DDeviceContext(), nullptr/*swapChainResources.GetBackBufferRenderTargetView()*/))
		, _inputSink(nullptr)
	{
		_render->SetDisplayOrientation(DOFromDegrees(ComputeDisplayRotation(_displayInformation->NativeOrientation, _displayInformation->CurrentOrientation)));
		_regOrientationChanged = _displayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(
			[&](DisplayInformation^ sender, Platform::Object^)
		{
			if (_inputSink)
			{
				_render->SetDisplayOrientation(DOFromDegrees(ComputeDisplayRotation(sender->NativeOrientation, sender->CurrentOrientation)));
			}
		});

		_regSizeChanged = _coreWindow->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(
			[&](CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
		{
			if (_inputSink)
			{
				float dpi = _displayInformation->LogicalDpi;
				_inputSink->GetDesktop()->Resize(PixelsFromDips(sender->Bounds.Width, dpi), PixelsFromDips(sender->Bounds.Height, dpi));
			}
		});

		_regPointerMoved = _coreWindow->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
			[&](CoreWindow^ sender, PointerEventArgs^ args)
		{
			if (_inputSink)
			{
				float dpi = _displayInformation->LogicalDpi;
				args->Handled = _inputSink->ProcessMouse(
					PixelsFromDips(args->CurrentPoint->Position.X, dpi),
					PixelsFromDips(args->CurrentPoint->Position.Y, dpi), 0, UI::MSGMOUSEMOVE);
			}
		});

		_regPointerPressed = _coreWindow->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
			[&](CoreWindow^ sender, PointerEventArgs^ args)
		{
			if (_inputSink)
			{
				float dpi = _displayInformation->LogicalDpi;
				args->Handled = _inputSink->ProcessMouse(
					PixelsFromDips(args->CurrentPoint->Position.X, dpi),
					PixelsFromDips(args->CurrentPoint->Position.Y, dpi), 0, UI::MSGLBUTTONDOWN);
			}
		});

		_regPointerReleased = _coreWindow->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
			[&](CoreWindow^ sender, PointerEventArgs^ args)
		{
			if (_inputSink)
			{
				float dpi = _displayInformation->LogicalDpi;
				args->Handled = _inputSink->ProcessMouse(
					PixelsFromDips(args->CurrentPoint->Position.X, dpi),
					PixelsFromDips(args->CurrentPoint->Position.Y, dpi), 0, UI::MSGLBUTTONUP);
			}
		});

		_coreWindow->PointerWheelChanged += ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(
			[&](CoreWindow^ sender, PointerEventArgs^ args)
		{
			if (_inputSink)
			{
				int delta = args->CurrentPoint->Properties->MouseWheelDelta;
				float dpi = _displayInformation->LogicalDpi;
				args->Handled = _inputSink->ProcessMouse(
					PixelsFromDips(args->CurrentPoint->Position.X, dpi),
					PixelsFromDips(args->CurrentPoint->Position.Y, dpi), (float) delta / 120.f, UI::MSGMOUSEWHEEL);
			}
		});

		_regKeyDown = _coreWindow->KeyDown += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
			[&](CoreWindow^ sender, KeyEventArgs^ args)
		{
			if (_inputSink)
			{
				args->Handled = _inputSink->ProcessKeys(UI::MSGKEYDOWN, MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey));
			}
		});

		_regKeyUp = _coreWindow->KeyUp += ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(
			[&](CoreWindow^ sender, KeyEventArgs^ args)
		{
			if (_inputSink)
			{
				args->Handled = _inputSink->ProcessKeys(UI::MSGKEYUP, MapWinStoreKeyCode(args->VirtualKey, args->KeyStatus.IsExtendedKey));
			}
		});

		_regCharacterReceived = _coreWindow->CharacterReceived += ref new TypedEventHandler<CoreWindow ^, CharacterReceivedEventArgs ^>(
			[&](CoreWindow^ sender, CharacterReceivedEventArgs^ args)
		{
			if (_inputSink)
			{
				args->Handled = _inputSink->ProcessText(args->KeyCode);
			}
		});
	}

	~StoreAppWindow()
	{
		_coreWindow->CharacterReceived -= _regCharacterReceived;
		_coreWindow->KeyUp -= _regKeyUp;
		_coreWindow->KeyDown -= _regKeyDown;
		_coreWindow->PointerReleased -= _regPointerReleased;
		_coreWindow->PointerPressed -= _regPointerMoved;
		_coreWindow->PointerMoved -= _regPointerMoved;
		_coreWindow->SizeChanged -= _regSizeChanged;

		_displayInformation->OrientationChanged -= _regOrientationChanged;
	}

	// AppWindow
	UI::IClipboard& GetClipboard() override
	{
		return _clipboard;
	}

	UI::IInput& GetInput() override
	{
		return _input;
	}

	IRender& GetRender() override
	{
		return *_render;
	}

	unsigned int GetPixelWidth() override
	{
		float dpi = _displayInformation->LogicalDpi;
		return (unsigned int)PixelsFromDips(_coreWindow->Bounds.Width, dpi);
	}

	unsigned int GetPixelHeight() override
	{
		float dpi = _displayInformation->LogicalDpi;
		return (unsigned int)PixelsFromDips(_coreWindow->Bounds.Height, dpi);
	}

	void SetInputSink(UI::LayoutManager *inputSink) override
	{
		_inputSink = inputSink;
	}

private:
	DisplayInformation^ _displayInformation;
	Windows::Foundation::EventRegistrationToken _regOrientationChanged;

	Platform::Agile<CoreWindow> _coreWindow;
	DX::DeviceResources &_deviceResources;
	StoreAppClipboard _clipboard;
	StoreAppInput _input;
	std::unique_ptr<IRender> _render;
	UI::LayoutManager *_inputSink;

	Windows::Foundation::EventRegistrationToken _regSizeChanged;
	Windows::Foundation::EventRegistrationToken _regPointerMoved;
	Windows::Foundation::EventRegistrationToken _regPointerPressed;
	Windows::Foundation::EventRegistrationToken _regPointerReleased;
	Windows::Foundation::EventRegistrationToken _regKeyDown;
	Windows::Foundation::EventRegistrationToken _regKeyUp;
	Windows::Foundation::EventRegistrationToken _regCharacterReceived;
};

// Called when the CoreWindow object is created (or re-created).
void FrameworkView::SetWindow(CoreWindow^ coreWindow)
{
	m_window = coreWindow;

	coreWindow->SizeChanged +=
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &FrameworkView::OnWindowSizeChanged);

	coreWindow->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &FrameworkView::OnVisibilityChanged);

	coreWindow->Closed +=
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &FrameworkView::OnWindowClosed);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &FrameworkView::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &FrameworkView::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &FrameworkView::OnDisplayContentsInvalidated);

	assert(m_deviceResources);
	assert(!m_swapChainResources);
	m_swapChainResources = std::make_shared<DX::SwapChainResources>(*m_deviceResources, coreWindow);
}

// Initializes scene resources, or loads a previously saved app state.
void FrameworkView::Load(Platform::String^ entryPoint)
{
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
static void PrepareForRender(DX::DeviceResources &deviceResources, DX::SwapChainResources &swapChainResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	context->DiscardView(swapChainResources.GetBackBufferRenderTargetView());

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { swapChainResources.GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, nullptr/*swapChainResources.GetDepthStencilView()*/);

	context->ClearRenderTargetView(swapChainResources.GetBackBufferRenderTargetView(), DirectX::Colors::YellowGreen/*Transparent*/);
}

// This method is called after the window becomes active.
void FrameworkView::Run()
{
	// todo: theese have to be recreated on device lost
	StoreAppWindow appWindow(m_window.Get(), *m_deviceResources, *m_swapChainResources);
	TzodView view(*_fs, _logger, _app, appWindow);

	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			m_window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			m_timer.Tick([&]()
			{
				float dt = (float)m_timer.GetElapsedSeconds();
				view.Step(dt);
				_app.Step(dt);
			});

			// Don't try to render anything before the first Update.
			if (m_timer.GetFrameCount() > 0)
			{
				PrepareForRender(*m_deviceResources, *m_swapChainResources);
				view.Render(appWindow);

				// The first argument instructs DXGI to block until VSync, putting the application
				// to sleep until the next VSync. This ensures we don't waste any cycles rendering
				// frames that will never be displayed to the screen.
				HRESULT hr = m_swapChainResources->GetSwapChain()->Present(1, 0);

				// If the device was removed either by a disconnection or a driver upgrade, we 
				// must recreate all device resources.
				if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
				{
					HandleDeviceLost();
				}
				else if (FAILED(hr))
				{
					throw Platform::Exception::CreateException(hr);
				}
			}
		}
		else
		{
			m_window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void FrameworkView::Uninitialize()
{
}


// Application lifecycle event handlers.

void FrameworkView::OnAppViewActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// Run() won't start until the CoreWindow is activated.
	applicationView->CoreWindow->Activate();
}

void FrameworkView::OnAppSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	create_task([this, deferral]()
	{
		m_deviceResources->Trim();

		// Insert your code here.

		deferral->Complete();
	});
}

void FrameworkView::OnAppResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.

	// Insert your code here.
}


// Window event handlers.

void FrameworkView::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_swapChainResources->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
}

void FrameworkView::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void FrameworkView::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

// DisplayInformation event handlers.

void FrameworkView::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	m_swapChainResources->SetDpi(sender->LogicalDpi);

	// When the display DPI changes, the logical size of the window (measured in Dips) also changes and needs to be updated.
	m_swapChainResources->SetLogicalSize(Size(m_window->Bounds.Width, m_window->Bounds.Height));
}

void FrameworkView::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	m_swapChainResources->SetCurrentOrientation(sender->CurrentOrientation);
}

void FrameworkView::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	if (!m_deviceResources->ValidateDevice())
	{
		HandleDeviceLost();
	}
}

void FrameworkView::HandleDeviceLost()
{
	m_swapChainResources.reset();
	m_deviceResources.reset(new DX::DeviceResources());
	m_swapChainResources.reset(new DX::SwapChainResources(*m_deviceResources, m_window.Get()));
}
