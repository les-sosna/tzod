#include "pch.h"
#include "FrameworkView.h"
#include "DeviceResources.h"
#include "SwapChainResources.h"
#include "StoreAppWindow.h"

#include <app/tzod.h>
#include <app/View.h>

using namespace wtzod;

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Graphics::Display;

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
				_app.Step(dt);
			});

			auto context = m_deviceResources->GetD3DDeviceContext();
			ID3D11RenderTargetView *const targets[1] = { m_swapChainResources->GetBackBufferRenderTargetView() };
			context->OMSetRenderTargets(1, targets, nullptr);
			context->DiscardView(m_swapChainResources->GetBackBufferRenderTargetView());

			// TODO: for night mode use DirectX::Colors::Transparent
			context->ClearRenderTargetView(m_swapChainResources->GetBackBufferRenderTargetView(), DirectX::XMVECTORF32{ 0, 0, 0, 1 });

			_view->Render(*_appWindow, _appWindow->GetPixelWidth(), _appWindow->GetPixelHeight(), _appWindow->GetLayoutScale());

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
	if (!m_swapChainResources->SetLogicalSize(Windows::Foundation::Size(sender->Bounds.Width, sender->Bounds.Height)))
	{
		HandleDeviceLost();
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
	if (!m_swapChainResources->SetDpi(sender->LogicalDpi) ||
		!m_swapChainResources->SetLogicalSize(Windows::Foundation::Size(m_window->Bounds.Width, m_window->Bounds.Height)))
	{
		HandleDeviceLost();
	}
}

void FrameworkView::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	if (!m_swapChainResources->SetCurrentOrientation(sender->CurrentOrientation))
	{
		HandleDeviceLost();
	}
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
	_view.reset();
	_appWindow.reset();

	m_swapChainResources.reset();
	m_deviceResources.reset(new DX::DeviceResources());
	m_swapChainResources.reset(new DX::SwapChainResources(*m_deviceResources, m_window.Get()));

	_appWindow.reset(new StoreAppWindow(m_window.Get(), *m_deviceResources, *m_swapChainResources));
	_view.reset(new TzodView(_fs, _logger, _app, *_appWindow));
}
