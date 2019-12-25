#include "inc/platetc/UIInputRenderingController.h"
#include <plat/Clipboard.h>
#include <ui/DataContext.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>
#include <ui/Navigation.h>
#include <ui/StateContext.h>
#include <ui/Window.h>
#include <plat/Input.h>
#include <plat/Keys.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

UIInputRenderingController::UIInputRenderingController(FS::FileSystem& fs,
                                                       TextureManager &textureManager,
                                                       UI::TimeStepManager &timeStepManager,
                                                       std::shared_ptr<UI::Window> desktop)
	: _fs(fs)
	, _textureManager(textureManager)
	, _timeStepManager(timeStepManager)
	, _desktop(desktop)
{
}

UIInputRenderingController::~UIInputRenderingController()
{
}

void UIInputRenderingController::TimeStep(float dt, const Plat::Input& input)
{
	_inputContext.ReadInput(input);
	_timeStepManager.TimeStep(_desktop, dt, input);
}

bool UIInputRenderingController::OnChar(Plat::AppWindow& appWindow, unsigned int codepoint)
{
	return _inputContext.ProcessText(_textureManager, _desktop, appWindow, UI::TextOperation::CharacterInput, codepoint);
}

bool UIInputRenderingController::OnKey(Plat::AppWindow& appWindow, Plat::Key key, Plat::Msg action)
{
	if (HandleClipboardShortcuts(appWindow, key, action))
		return true;

	UI::DataContext dataContext;
	UI::LayoutContext layoutContext(1.f, appWindow.GetLayoutScale(), vec2d{}, appWindow.GetPixelSize(), true /* enabled */, _inputContext.GetMainWindowActive());
	return _inputContext.ProcessKeys(
		appWindow.GetInput(),
		_textureManager,
		_desktop,
		layoutContext,
		dataContext,
		action,
		key,
		_timeStepManager.GetTime());
}

bool UIInputRenderingController::OnPointer(Plat::AppWindow& appWindow, Plat::PointerType pointerType, Plat::Msg action, vec2d pxPointerPos, vec2d pxPointerOffset, int buttons, unsigned int pointerID)
{
	UI::DataContext dataContext;
	return _inputContext.ProcessPointer(
		appWindow.GetInput(),
		_textureManager,
		_desktop,
		UI::LayoutContext(1.f, appWindow.GetLayoutScale(), vec2d{}, appWindow.GetPixelSize(), true /* enabled */, _inputContext.GetMainWindowActive()),
		dataContext,
		pxPointerPos,
		pxPointerOffset,
		action,
		buttons,
		pointerType,
		pointerID,
		_timeStepManager.GetTime());
}

bool UIInputRenderingController::OnSystemNavigationBack(Plat::AppWindow& appWindow)
{
	UI::DataContext dataContext;
	return _inputContext.ProcessSystemNavigationBack(
		_textureManager,
		_desktop,
		UI::LayoutContext(1.f, appWindow.GetLayoutScale(), vec2d{}, appWindow.GetPixelSize(), true /* enabled */, _inputContext.GetMainWindowActive()),
		dataContext);
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

void UIInputRenderingController::OnRefresh(Plat::AppWindow& appWindow, RenderBinding &rb)
{
	IRender& render = appWindow.GetRender();
	vec2d pxWindowSize = appWindow.GetPixelSize();

	auto displayWidth = static_cast<unsigned int>(pxWindowSize.x);
	auto displayHeight = static_cast<unsigned int>(pxWindowSize.y);
	render.Begin(displayWidth, displayHeight, DOFromDegrees(appWindow.GetDisplayRotation()));
	render.SetViewport({ 0, 0, (int)displayWidth, (int)displayHeight });

	ImageCache imageCache;
	rb.Update(RenderBindingEnv{ _fs, _textureManager, imageCache, render });
	RenderContext rc(_textureManager, rb, render, displayWidth, displayHeight);

	UI::DataContext dataContext;
	UI::LayoutContext layoutContext(1.f, appWindow.GetLayoutScale(), vec2d{}, pxWindowSize, true /* enabled */, _inputContext.GetMainWindowActive());
	UI::RenderSettings rs{ appWindow.GetInput(), _inputContext, rc, _textureManager, _timeStepManager.GetTime() };

	UI::RenderUIRoot(_desktop, rs, layoutContext, dataContext, UI::StateContext());

	bool hoverTextSink = false;
	for (auto &wnd : rs.hoverPath)
	{
		if (wnd->HasTextSink())
		{
			hoverTextSink = true;
			break;
		}
	}

	appWindow.SetCanNavigateBack(CanNavigateBack(_textureManager, *_desktop, layoutContext, dataContext));

	Plat::MouseCursor mouseCursor = hoverTextSink ? Plat::MouseCursor::IBeam : Plat::MouseCursor::Arrow;
	if (_mouseCursor != mouseCursor)
	{
		_mouseCursor = mouseCursor;
		appWindow.SetMouseCursor(mouseCursor);
	}

#ifndef NDEBUG
	for (auto &id2pos : rs.ic.GetLastPointerLocation())
	{
		vec2d pos = Vec2dFloor(id2pos.second);
		FRECT dst = { pos.x - 4, pos.y - 4, pos.x + 4, pos.y + 4 };
		rs.rc.DrawSprite(dst, 0U, 0xffffffff, 0U);
	}
#endif

	render.End();
	appWindow.Present();
}

bool UIInputRenderingController::HandleClipboardShortcuts(Plat::AppWindow& appWindow, Plat::Key key, Plat::Msg action)
{
	if (action == Plat::Msg::KeyPressed)
	{
		auto& input = appWindow.GetInput();
		bool shift = input.IsKeyPressed(Plat::Key::LeftShift) || input.IsKeyPressed(Plat::Key::RightShift);
		bool control = input.IsKeyPressed(Plat::Key::LeftCtrl) || input.IsKeyPressed(Plat::Key::RightCtrl);

		switch (key)
		{
		case Plat::Key::Insert:
			if (shift)
			{
				Paste(appWindow);
				return true;
			}
			else if (control)
			{
				Copy(appWindow);
				return true;
			}
			break;
		case Plat::Key::V:
			if (control)
			{
				Paste(appWindow);
				return true;
			}
			break;
		case Plat::Key::C:
			if (control)
			{
				Copy(appWindow);
				return true;
			}
			break;
		case Plat::Key::X:
			if (control)
			{
				//if (0 != GetSelLength())
				Cut(appWindow);
				return true;
			}
			break;
		case Plat::Key::Delete:
			if (shift)
			{
				Cut(appWindow);
				return true;
			}
			break;
		default:
			return false;
		}
	}
	return false;
}

void UIInputRenderingController::Cut(Plat::AppWindow& appWindow)
{
	_inputContext.ProcessText(_textureManager, _desktop, appWindow, UI::TextOperation::ClipboardCut);
}

void UIInputRenderingController::Copy(Plat::AppWindow& appWindow)
{
	_inputContext.ProcessText(_textureManager, _desktop, appWindow, UI::TextOperation::ClipboardCopy);
}

void UIInputRenderingController::Paste(Plat::AppWindow& appWindow)
{
	_inputContext.ProcessText(_textureManager, _desktop, appWindow, UI::TextOperation::ClipboardPaste);
}
