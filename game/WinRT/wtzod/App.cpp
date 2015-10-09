#include "pch.h"
#include "App.h"
#include "Content\Sample3DSceneRenderer.h"
#include "DeviceResources.h"
#include "SwapChainResources.h"

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

App::App()
	: m_windowClosed(false)
	, m_windowVisible(true)
{
}

App::~App()
{
}

// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ coreApplicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	coreApplicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &App::OnResuming);

	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	assert(!m_deviceResources);
	m_deviceResources = std::make_shared<DX::DeviceResources>();
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ coreWindow)
{
	m_window = coreWindow;

	coreWindow->SizeChanged +=
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnWindowSizeChanged);

	coreWindow->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);

	coreWindow->Closed +=
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDisplayContentsInvalidated);

	assert(m_deviceResources);
	assert(!m_swapChainResources);
	m_swapChainResources = std::make_shared<DX::SwapChainResources>(*m_deviceResources, coreWindow);
}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(Platform::String^ entryPoint)
{
	if (m_sceneRenderer == nullptr)
	{
		m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer());
		m_sceneRenderer->SetDeviceResources(m_deviceResources.get());
		m_sceneRenderer->SetSwapChainResources(m_swapChainResources.get());
	}
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
static void PrepareForRender(DX::DeviceResources &deviceResources, DX::SwapChainResources &swapChainResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	// Set the 3D rendering viewport to target the entire screen.
	auto screenViewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		swapChainResources.GetRenderTargetSize().Width,
		swapChainResources.GetRenderTargetSize().Height
		);
	context->RSSetViewports(1, &screenViewport);

	// Discard the contents of the render target.
	// This is a valid operation only when the existing contents will be entirely
	// overwritten. If dirty or scroll rects are used, this call should be removed.
	context->DiscardView(swapChainResources.GetBackBufferRenderTargetView());

	// Discard the contents of the depth stencil.
	context->DiscardView(swapChainResources.GetDepthStencilView());

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { swapChainResources.GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, swapChainResources.GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
	context->ClearRenderTargetView(swapChainResources.GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(swapChainResources.GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

// This method is called after the window becomes active.
void App::Run()
{
	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			// Update scene objects.
			m_timer.Tick([&]()
			{
				// TODO: Replace this with your app's content update functions.
				m_sceneRenderer->Update(m_timer);
			});

			// Don't try to render anything before the first Update.
			if (m_timer.GetFrameCount() > 0)
			{
				PrepareForRender(*m_deviceResources, *m_swapChainResources);
				
				// Render the scene objects.
				// TODO: Replace this with your app's content rendering functions.
				m_sceneRenderer->Render();

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
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

// Application lifecycle event handlers.

void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// Run() won't start until the CoreWindow is activated.
	CoreWindow::GetForCurrentThread()->Activate();
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
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

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.

	// Insert your code here.
}

// Window event handlers.

void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_swapChainResources->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
	m_sceneRenderer->SetSwapChainResources(m_swapChainResources.get());
}

void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

// DisplayInformation event handlers.

void App::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	// When the display DPI changes, the logical size of the window (measured in Dips) also changes and needs to be updated.
	Size logicalSize(m_window->Bounds.Width, m_window->Bounds.Height);
	m_swapChainResources->SetDpi(sender->LogicalDpi, logicalSize);
	m_sceneRenderer->SetSwapChainResources(m_swapChainResources.get());
}

void App::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	m_swapChainResources->SetCurrentOrientation(sender->CurrentOrientation);
	m_sceneRenderer->SetSwapChainResources(m_swapChainResources.get());
}

void App::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	if (!m_deviceResources->ValidateDevice())
	{
		HandleDeviceLost();
	}
}

void App::HandleDeviceLost()
{
	m_sceneRenderer->SetSwapChainResources(nullptr);
	m_sceneRenderer->SetDeviceResources(nullptr);

	m_swapChainResources.reset();
	m_deviceResources.reset(new DX::DeviceResources());
	m_swapChainResources.reset(new DX::SwapChainResources(*m_deviceResources, m_window.Get()));

	m_sceneRenderer->SetDeviceResources(m_deviceResources.get());
	m_sceneRenderer->SetSwapChainResources(m_swapChainResources.get());
}
