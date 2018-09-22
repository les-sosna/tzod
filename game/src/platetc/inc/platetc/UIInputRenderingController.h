#pragma once
#include <plat/AppWindow.h>
#include <ui/InputContext.h>

namespace UI
{
	class TimeStepManager;
}

class UIInputRenderingController : public AppWindowInputSink
{
public:
	UIInputRenderingController(
		AppWindow &appWindow,
		TextureManager &textureManager,
		UI::TimeStepManager &timeStepManager,
		std::shared_ptr<UI::Window> desktop);
	~UIInputRenderingController();

	void TimeStep(float dt);

	// AppWindowInputSink
	bool OnChar(unsigned int codepoint) override;
	bool OnKey(UI::Key key, UI::Msg action) override;
	bool OnPointer(UI::PointerType pointerType, UI::Msg action, vec2d pxPointerPos, vec2d pxPointerOffset, int buttons, unsigned int pointerID) override;
	bool OnSystemNavigationBack() override;
	void OnRefresh() override;

private:
	bool HandleClipboardShortcuts(UI::Key key, UI::Msg action);
	void Cut();
	void Copy();
	void Paste();

	AppWindow &_appWindow;
	MouseCursor _mouseCursor = MouseCursor::Arrow;
	UI::InputContext _inputContext;
	TextureManager &_textureManager;
	UI::TimeStepManager &_timeStepManager;
	std::shared_ptr<UI::Window> _desktop;
};
