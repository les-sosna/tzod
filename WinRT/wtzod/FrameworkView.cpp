#include "pch.h"
#include "FrameworkView.h"
#include "DeviceResources.h"
#include "DeviceResources12.h"
#include "DirectXHelper.h"
#include "StoreAppWindow.h"
#include "DisplayOrientation.h"
#include <app/tzod.h>
#include <video/RenderBase.h>

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
}

FrameworkView::~FrameworkView()
{
}

// The first method called when the IFrameworkView is being created.
void FrameworkView::Initialize(CoreApplicationView^ coreApplicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	CoreApplication::Suspending += ref new Windows::Foundation::EventHandler<SuspendingEventArgs^>(this, &FrameworkView::OnAppSuspending);
	CoreApplication::Resuming += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &FrameworkView::OnAppResuming);
	coreApplicationView->Activated +=
		ref new Windows::Foundation::TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &FrameworkView::OnAppViewActivated);
}

// Called when the CoreWindow object is created (or re-created).
void FrameworkView::SetWindow(CoreWindow^ coreWindow)
{
	m_window = coreWindow;
	_appWindow.reset(new StoreAppWindow(m_window.Get()));

//	DisplayInformation::DisplayContentsInvalidated +=
//		ref new Windows::Foundation::TypedEventHandler<DisplayInformation^, Object^>(this, &FrameworkView::OnDisplayContentsInvalidated);
}

// Initializes scene resources, or loads a previously saved app state.
void FrameworkView::Load(Platform::String^ entryPoint)
{
}

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

// This method is called after the window becomes active.
void FrameworkView::Run()
{
	auto refreshAndPresent = [this](auto& deviceResources)
	{
		if (!deviceResources || deviceResources->IsDeviceRemoved())
		{
			using ElementType = std::remove_reference<decltype(deviceResources)>::type::element_type;
			deviceResources.reset();
			deviceResources.reset(new ElementType(m_window.Get()));
		}

		vec2d pxSize = _appWindow->GetPixelSize();
		_view.GetAppWindowInputSink().OnRefresh(
			*_appWindow,
			deviceResources->GetRender((int)pxSize.x, (int)pxSize.y, DOFromDegrees(_appWindow->GetDisplayRotation())),
			deviceResources->GetRenderBinding());
		deviceResources->Present();
	};

	while (!_appWindow->ShouldClose())
	{
		if (_appWindow->IsVisible())
		{
			if (true)
				refreshAndPresent(_deviceResources);
			else
				refreshAndPresent(_deviceResources12);

			_appWindow->PollEvents(_view.GetAppWindowInputSink(), CoreProcessEventsOption::ProcessAllIfPresent);

			m_timer.Tick([&]()
			{
				float dt = (float)m_timer.GetElapsedSeconds();
				_view.Step(_app, dt, _appWindow->GetInput());
			});
		}
		else
		{
			_appWindow->PollEvents(_view.GetAppWindowInputSink(), CoreProcessEventsOption::ProcessOneAndAllPending);
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
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	_deviceResources.reset();
	_deviceResources12.reset();
	_app.SaveConfig();

	deferral->Complete();
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
	if (_deviceResources && !_deviceResources->ValidateDevice())
	{
		_deviceResources.reset();
	}
}
*/
