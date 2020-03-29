#pragma once
#include "StoreAppClipboard.h"
#include "StoreAppInput.h"
#include <plat/AppWindow.h>

class StoreAppWindow final
	: public Plat::AppWindow
{
public:
	StoreAppWindow(Windows::UI::Core::CoreWindow^ coreWindow);
	~StoreAppWindow();

	bool ShouldClose() const { return _shouldClose; }
	bool IsVisible() const { return _visible; }
	void PollEvents(Plat::AppWindowInputSink& inputSink, Windows::UI::Core::CoreProcessEventsOption options);

	// AppWindow
	int GetDisplayRotation() const override { return _displayRotation; }
	vec2d GetPixelSize() const override;
	float GetLayoutScale() const override;
	Plat::Clipboard& GetClipboard() override;
	Plat::Input& GetInput() override;
	void SetCanNavigateBack(bool canNavigateBack) override;
	void SetMouseCursor(Plat::MouseCursor mouseCursor) override;

private:
	Windows::UI::Input::GestureRecognizer ^_gestureRecognizer;
	Windows::UI::Core::SystemNavigationManager ^_systemNavigationManager;
	Windows::Graphics::Display::DisplayInformation ^_displayInformation;
	Windows::Foundation::EventRegistrationToken _regBackRequested;
	Windows::Foundation::EventRegistrationToken _regDpiChanged;
	Windows::Foundation::EventRegistrationToken _regOrientationChanged;

	Platform::Agile<Windows::UI::Core::CoreWindow> _coreWindow;
	Windows::UI::Core::CoreCursor ^_cursorArrow;
	Windows::UI::Core::CoreCursor ^_cursorIBeam;
	Plat::MouseCursor _mouseCursor = Plat::MouseCursor::Arrow;
	StoreAppClipboard _clipboard;
	StoreAppInput _input;

	int _displayRotation;
	bool _shouldClose = false;
	bool _visible = true;

	// CoreWindow event registrations
	Windows::Foundation::EventRegistrationToken _regVisibilityChanged;
	Windows::Foundation::EventRegistrationToken _regClosed;
	Windows::Foundation::EventRegistrationToken _regSizeChanged;
	Windows::Foundation::EventRegistrationToken _regPointerMoved;
	Windows::Foundation::EventRegistrationToken _regPointerPressed;
	Windows::Foundation::EventRegistrationToken _regPointerReleased;
	Windows::Foundation::EventRegistrationToken _regKeyDown;
	Windows::Foundation::EventRegistrationToken _regKeyUp;
	Windows::Foundation::EventRegistrationToken _regCharacterReceived;

	// temp data only set during PollEvents
	std::shared_ptr<StoreAppWindow*> _self = std::make_shared<StoreAppWindow*>();
	std::shared_ptr<Plat::AppWindowInputSink*> _inputSink = std::make_shared<Plat::AppWindowInputSink*>();
};
