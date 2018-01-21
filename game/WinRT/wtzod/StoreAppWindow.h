#pragma once
#include "StoreAppClipboard.h"
#include "StoreAppInput.h"
#include <plat/AppWindow.h>

class SwapChainResources;
namespace DX
{
	class DeviceResources;
}

class StoreAppWindow : public AppWindow
{
public:
	StoreAppWindow(Windows::UI::Core::CoreWindow^ coreWindow, DX::DeviceResources &deviceResources, SwapChainResources &swapChainResources);
	~StoreAppWindow();

	// AppWindow
	AppWindowInputSink* GetInputSink() const override { return *_inputSink; }
	void SetInputSink(AppWindowInputSink *inputSink) override { *_inputSink = inputSink; }
	vec2d GetPixelSize() const override;
	float GetLayoutScale() const override;
	UI::IClipboard& GetClipboard() override;
	UI::IInput& GetInput() override;
	IRender& GetRender() override;
	void SetCanNavigateBack(bool canNavigateBack) override;
	void SetMouseCursor(MouseCursor mouseCursor) override;
	void Present() override;
	void MakeCurrent() override {}

private:
	Windows::UI::Input::GestureRecognizer ^_gestureRecognizer;
	Windows::UI::Core::SystemNavigationManager ^_systemNavigationManager;
	Windows::Graphics::Display::DisplayInformation ^_displayInformation;
	Windows::Foundation::EventRegistrationToken _regOrientationChanged;
	Windows::Foundation::EventRegistrationToken _regBackRequested;

	Platform::Agile<Windows::UI::Core::CoreWindow> _coreWindow;
	Windows::UI::Core::CoreCursor ^_cursorArrow;
	Windows::UI::Core::CoreCursor ^_cursorIBeam;
	MouseCursor _mouseCursor = MouseCursor::Arrow;
	DX::DeviceResources &_deviceResources;
	SwapChainResources &_swapChainResources;
	StoreAppClipboard _clipboard;
	StoreAppInput _input;
	std::unique_ptr<IRender> _render;
	std::shared_ptr<AppWindowInputSink *> _inputSink = std::make_shared<AppWindowInputSink*>();

	Windows::Foundation::EventRegistrationToken _regPointerMoved;
	Windows::Foundation::EventRegistrationToken _regPointerPressed;
	Windows::Foundation::EventRegistrationToken _regPointerReleased;
	Windows::Foundation::EventRegistrationToken _regKeyDown;
	Windows::Foundation::EventRegistrationToken _regKeyUp;
	Windows::Foundation::EventRegistrationToken _regCharacterReceived;
};
