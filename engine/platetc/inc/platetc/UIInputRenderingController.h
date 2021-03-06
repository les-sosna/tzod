#pragma once
#include <plat/AppWindow.h>
#include <ui/InputContext.h>
#include <video/RenderBinding.h>

namespace UI
{
	class TimeStepManager;
}

class UIInputRenderingController final
	: public Plat::AppWindowInputSink
{
public:
	UIInputRenderingController(
		FS::FileSystem& fs,
		TextureManager &textureManager,
		UI::TimeStepManager &timeStepManager,
		std::shared_ptr<UI::Window> desktop);
	~UIInputRenderingController();

	void TimeStep(float dt, const Plat::Input& input);

	// AppWindowInputSink
	bool OnChar(Plat::AppWindow& appWindow, unsigned int codepoint) override;
	bool OnKey(Plat::AppWindow& appWindow, Plat::Key key, Plat::Msg action) override;
	bool OnPointer(Plat::AppWindow& appWindow, Plat::PointerType pointerType, Plat::Msg action, vec2d pxPointerPos, vec2d pxPointerOffset, int buttons, unsigned int pointerID) override;
	bool OnSystemNavigationBack(Plat::AppWindow& appWindow) override;
	void OnRefresh(Plat::AppWindow& appWindow, IRender& render, RenderBinding& rb) override;

private:
	bool HandleClipboardShortcuts(Plat::AppWindow& appWindow, Plat::Key key, Plat::Msg action);
	void Cut(Plat::AppWindow& appWindow);
	void Copy(Plat::AppWindow& appWindow);
	void Paste(Plat::AppWindow& appWindow);

	Plat::MouseCursor _mouseCursor = Plat::MouseCursor::Arrow;
	UI::InputContext _inputContext;
	FS::FileSystem& _fs;
	TextureManager &_textureManager;
	UI::TimeStepManager &_timeStepManager;
	std::shared_ptr<UI::Window> _desktop;
};
