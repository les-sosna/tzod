#include "pch.h"
#include "FrameworkView.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "StoreAppWindow.h"
#include "DisplayOrientation.h"

#include <app/tzod.h>
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

FrameworkView::FrameworkView(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, TzodApp &app)
	: _fs(fs)
	, _logger(logger)
	, _app(app)
	, _view(_fs, _logger, _app, nullptr)
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

//	DisplayInformation::DisplayContentsInvalidated +=
//		ref new Windows::Foundation::TypedEventHandler<DisplayInformation^, Object^>(this, &FrameworkView::OnDisplayContentsInvalidated);

	HandleDeviceLost();
}

// Initializes scene resources, or loads a previously saved app state.
void FrameworkView::Load(Platform::String^ entryPoint)
{
}

// This method is called after the window becomes active.
void FrameworkView::Run()
{
	while (!_appWindow->ShouldClose())
	{
		if (!_appWindow || _appWindow->IsVisible())
		{
			if (!_appWindow || _appWindow->IsDeviceRemoved())
			{
				HandleDeviceLost();
			}

			_view.GetAppWindowInputSink().OnRefresh(*_appWindow);

			_appWindow->PollEvents(_view.GetAppWindowInputSink());

			m_timer.Tick([&]()
			{
				float dt = (float)m_timer.GetElapsedSeconds();
				_view.Step(_app, dt, _appWindow->GetInput());
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
//		m_deviceResources->Trim();

		_app.SaveConfig();

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

/*
void FrameworkView::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	if (!m_deviceResources->ValidateDevice())
	{
		HandleDeviceLost();
	}
}
*/
void FrameworkView::HandleDeviceLost()
{
	_appWindow.reset();
	_appWindow.reset(new StoreAppWindow(m_window.Get()));
}
