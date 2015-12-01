#pragma once

#include "pch.h"
#include "StepTimer.h"

// todo: move to main
#include <app/tzod.h>
#include <ui/ConsoleBuffer.h>

namespace DX
{
	class DeviceResources;
	class SwapChainResources;
}

namespace FS
{
	class FileSystem;
}

namespace wtzod
{
	class Sample3DSceneRenderer;

	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	ref class FrameworkView sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	public:
		FrameworkView();
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

		// Window event handlers.
		void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
		void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

		// DisplayInformation event handlers.
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

	private:
		// todo: move out of view class
		std::shared_ptr<FS::FileSystem> _fs;
		UI::ConsoleBuffer _logger;
		TzodApp _app;

		Platform::Agile<Windows::UI::Core::CoreWindow>  m_window;
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		std::shared_ptr<DX::SwapChainResources> m_swapChainResources;
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		bool m_windowClosed;
		bool m_windowVisible;
		
		void HandleDeviceLost();
	};
}
