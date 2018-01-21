#include "pch.h"
#include "FrameworkView.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "StoreAppWindow.h"
#include "DisplayOrientation.h"

#include <app/tzod.h>
#include <app/View.h>
#include <video/SwapChainResources.h>

using namespace wtzod;

using namespace concurrency;
using namespace Microsoft::WRL;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Graphics::Display;


static bool IsDeviceLost(HRESULT hr)
{
	return (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET);
}

FrameworkView::FrameworkView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app)
	: m_windowClosed(false)
	, m_windowVisible(true)
	, _fs(fs)
	, _logger(logger)
	, _app(app)
{
	CoreApplication::Suspending += ref new Windows::Foundation::EventHandler<SuspendingEventArgs^>(this, &FrameworkView::OnAppSuspending);
	CoreApplication::Resuming += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &FrameworkView::OnAppResuming);
}

FrameworkView::~FrameworkView()
{
}

// The first method called when the IFrameworkView is being created.
void FrameworkView::Initialize(CoreApplicationView^ coreApplicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	coreApplicationView->Activated +=
		ref new Windows::Foundation::TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &FrameworkView::OnAppViewActivated);
}

// Called when the CoreWindow object is created (or re-created).
void FrameworkView::SetWindow(CoreWindow^ coreWindow)
{
	m_window = coreWindow;

	coreWindow->SizeChanged +=
		ref new Windows::Foundation::TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &FrameworkView::OnWindowSizeChanged);

	coreWindow->VisibilityChanged +=
		ref new Windows::Foundation::TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &FrameworkView::OnVisibilityChanged);

	coreWindow->Closed +=
		ref new Windows::Foundation::TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &FrameworkView::OnWindowClosed);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new Windows::Foundation::TypedEventHandler<DisplayInformation^, Object^>(this, &FrameworkView::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new Windows::Foundation::TypedEventHandler<DisplayInformation^, Object^>(this, &FrameworkView::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new Windows::Foundation::TypedEventHandler<DisplayInformation^, Object^>(this, &FrameworkView::OnDisplayContentsInvalidated);

	HandleDeviceLost();
}

// Initializes scene resources, or loads a previously saved app state.
void FrameworkView::Load(Platform::String^ entryPoint)
{
}

// This method is called after the window becomes active.
void FrameworkView::Run()
{
	while (!m_windowClosed)
	{
		if (m_windowVisible && _view != nullptr)
		{
			m_window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			m_timer.Tick([&]()
			{
				float dt = (float)m_timer.GetElapsedSeconds();
				if (_view)
				{
					_view->Step(dt);
				}
			});
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

		_app.Exit();

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
	HRESULT hr = m_swapChainResources->SetPixelSize(
		m_deviceResources->GetD3DDevice(),
		m_deviceResources->GetD3DDeviceContext(),
		_appWindow->GetPixelSize());

	if (IsDeviceLost(hr))
	{
		HandleDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
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
	// When the display DPI changes, the logical size of the window (measured in Dips) also changes and needs to be updated.

	HRESULT hr = m_swapChainResources->SetPixelSize(
		m_deviceResources->GetD3DDevice(),
		m_deviceResources->GetD3DDeviceContext(),
		_appWindow->GetPixelSize());

	if (IsDeviceLost(hr))
	{
		HandleDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
}

void FrameworkView::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	int rotationAngle = ComputeDisplayRotation(sender->NativeOrientation, sender->CurrentOrientation);

	HRESULT hr = m_swapChainResources->SetCurrentOrientation(
		m_deviceResources->GetD3DDevice(),
		m_deviceResources->GetD3DDeviceContext(), rotationAngle);

	if (IsDeviceLost(hr))
	{
		HandleDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
}

void FrameworkView::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	if (!m_deviceResources->ValidateDevice())
	{
		HandleDeviceLost();
	}
}

static ComPtr<IDXGISwapChain1> CreateSwapchainForCoreWindow(ID3D11Device *d3dDevice, CoreWindow ^coreWindow)
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

void FrameworkView::HandleDeviceLost()
{
	_view.reset();
	_appWindow.reset();

	m_swapChainResources.reset();
	m_deviceResources.reset(new DX::DeviceResources());

	ComPtr<IDXGISwapChain1> swapChain = CreateSwapchainForCoreWindow(m_deviceResources->GetD3DDevice(), m_window.Get());
	m_swapChainResources = std::make_unique<SwapChainResources>(swapChain.Get());

	_appWindow.reset(new StoreAppWindow(m_window.Get(), *m_deviceResources, *m_swapChainResources));
	_view.reset(new TzodView(_fs, _logger, _app, *_appWindow));

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();
	int rotationAngle = ComputeDisplayRotation(currentDisplayInformation->NativeOrientation, currentDisplayInformation->CurrentOrientation);
	m_swapChainResources->SetCurrentOrientation(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), rotationAngle);
	m_swapChainResources->SetPixelSize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), _appWindow->GetPixelSize());
}
