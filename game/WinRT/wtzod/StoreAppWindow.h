#pragma once
#include "StoreAppClipboard.h"
#include "StoreAppInput.h"
#include <ui/AppWindow.h>

namespace DX
{
	class DeviceResources;
	class SwapChainResources;
}

class StoreAppWindow : public AppWindow
{
public:
	StoreAppWindow(Windows::UI::Core::CoreWindow^ coreWindow, DX::DeviceResources &deviceResources, DX::SwapChainResources &swapChainResources);
	~StoreAppWindow();

	float GetPixelWidth() const;
	float GetPixelHeight() const;
	float GetLayoutScale() const;

	// AppWindow
	UI::IClipboard& GetClipboard() override;
	UI::IInput& GetInput() override;
	IRender& GetRender() override;
	void SetInputSink(UI::LayoutManager *inputSink) override;
	void SetMouseCursor(MouseCursor mouseCursor) override;
	void MakeCurrent() override {}

private:
	Windows::UI::Input::GestureRecognizer^ _gestureRecognizer;
	Windows::Graphics::Display::DisplayInformation^ _displayInformation;
	Windows::Foundation::EventRegistrationToken _regOrientationChanged;

	Platform::Agile<Windows::UI::Core::CoreWindow> _coreWindow;
	Windows::UI::Core::CoreCursor ^_cursorArrow;
	Windows::UI::Core::CoreCursor ^_cursorIBeam;
	DX::DeviceResources &_deviceResources;
	StoreAppClipboard _clipboard;
	StoreAppInput _input;
	std::unique_ptr<IRender> _render;
	std::shared_ptr<UI::LayoutManager*> _inputSink;

	Windows::Foundation::EventRegistrationToken _regSizeChanged;
	Windows::Foundation::EventRegistrationToken _regPointerMoved;
	Windows::Foundation::EventRegistrationToken _regPointerPressed;
	Windows::Foundation::EventRegistrationToken _regPointerReleased;
	Windows::Foundation::EventRegistrationToken _regKeyDown;
	Windows::Foundation::EventRegistrationToken _regKeyUp;
	Windows::Foundation::EventRegistrationToken _regCharacterReceived;
};
