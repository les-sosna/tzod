#pragma once
#include "StoreAppClipboard.h"
#include "StoreAppInput.h"
#include <plat/AppWindow.h>

class SwapChainResources;
namespace DX
{
	class DeviceResources;
}

class StoreAppWindow final
	: public Plat::AppWindow
{
public:
	StoreAppWindow(Windows::UI::Core::CoreWindow^ coreWindow, DX::DeviceResources &deviceResources, SwapChainResources &swapChainResources);
	~StoreAppWindow();

	// AppWindow
	Plat::AppWindowInputSink* GetInputSink() const override { return *_inputSink; }
	void SetInputSink(Plat::AppWindowInputSink *inputSink) override { *_inputSink = inputSink; }
	virtual int GetDisplayRotation() const override;
	vec2d GetPixelSize() const override;
	float GetLayoutScale() const override;
	Plat::Clipboard& GetClipboard() override;
	Plat::Input& GetInput() override;
	IRender& GetRender() override;
	void SetCanNavigateBack(bool canNavigateBack) override;
	void SetMouseCursor(Plat::MouseCursor mouseCursor) override;
	void Present() override;
	void MakeCurrent() override {}

private:
	Windows::UI::Input::GestureRecognizer ^_gestureRecognizer;
	Windows::UI::Core::SystemNavigationManager ^_systemNavigationManager;
	Windows::Graphics::Display::DisplayInformation ^_displayInformation;
	Windows::Foundation::EventRegistrationToken _regBackRequested;

	Platform::Agile<Windows::UI::Core::CoreWindow> _coreWindow;
	Windows::UI::Core::CoreCursor ^_cursorArrow;
	Windows::UI::Core::CoreCursor ^_cursorIBeam;
	Plat::MouseCursor _mouseCursor = Plat::MouseCursor::Arrow;
	DX::DeviceResources &_deviceResources;
	SwapChainResources &_swapChainResources;
	StoreAppClipboard _clipboard;
	StoreAppInput _input;
	std::unique_ptr<IRender> _render;
	std::shared_ptr<Plat::AppWindowInputSink *> _inputSink = std::make_shared<Plat::AppWindowInputSink*>();

	Windows::Foundation::EventRegistrationToken _regPointerMoved;
	Windows::Foundation::EventRegistrationToken _regPointerPressed;
	Windows::Foundation::EventRegistrationToken _regPointerReleased;
	Windows::Foundation::EventRegistrationToken _regKeyDown;
	Windows::Foundation::EventRegistrationToken _regKeyUp;
	Windows::Foundation::EventRegistrationToken _regCharacterReceived;
};
