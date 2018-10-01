#pragma once
#include <plat/AppWindow.h>
#include <ui/InputContext.h>

namespace UI
{
	class TimeStepManager;
}

class UIInputRenderingController final
	: public Plat::AppWindowInputSink
{
public:
	UIInputRenderingController(
		Plat::AppWindow &appWindow,
		TextureManager &textureManager,
		UI::TimeStepManager &timeStepManager,
		std::shared_ptr<UI::Window> desktop);
	~UIInputRenderingController();

	void TimeStep(float dt);

	// AppWindowInputSink
	bool OnChar(unsigned int codepoint) override;
	bool OnKey(Plat::Key key, Plat::Msg action) override;
	bool OnPointer(Plat::PointerType pointerType, Plat::Msg action, vec2d pxPointerPos, vec2d pxPointerOffset, int buttons, unsigned int pointerID) override;
	bool OnSystemNavigationBack() override;
	void OnRefresh() override;

private:
	bool HandleClipboardShortcuts(Plat::Key key, Plat::Msg action);
	void Cut();
	void Copy();
	void Paste();

	Plat::AppWindow &_appWindow;
	Plat::MouseCursor _mouseCursor = Plat::MouseCursor::Arrow;
	UI::InputContext _inputContext;
	TextureManager &_textureManager;
	UI::TimeStepManager &_timeStepManager;
	std::shared_ptr<UI::Window> _desktop;
};
