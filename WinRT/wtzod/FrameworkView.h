#pragma once

#include "pch.h"
#include "StepTimer.h"
#include <app/View.h>

class SwapChainResources;
namespace DX
{
	class DeviceResources;
}

namespace FS
{
	class FileSystem;
}

namespace Plat
{
	class ConsoleBuffer;
}

class TzodApp;
class TzodView;
class StoreAppWindow;

namespace wtzod
{
	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	ref class FrameworkView sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	internal:
		FrameworkView(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, TzodApp &app);

	public:
		virtual ~FrameworkView();

		// IFrameworkView
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ coreApplicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow^ coreWindow);
		virtual void Load(Platform::String^ entryPoint);
		virtual void Run();
		virtual void Uninitialize();

	protected:
		// Application lifecycle event handlers.
		void OnAppViewActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
		void OnAppSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
		void OnAppResuming(Platform::Object^ sender, Platform::Object^ args);

		// DisplayInformation event handlers.
//		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

	private:
		FS::FileSystem &_fs;
		Plat::ConsoleBuffer &_logger;
		TzodApp &_app;
		TzodView _view;

		Platform::Agile<Windows::UI::Core::CoreWindow>  m_window;
		std::unique_ptr<StoreAppWindow> _appWindow;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		std::unique_ptr<DX::DeviceResources> _deviceResources;
		std::unique_ptr<SwapChainResources> _swapChainResources;

		void HandleDeviceLost();
	};
}
