#include "inc/platetc/UIInputRenderingController.h"
#include <ui/DataContext.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>
#include <ui/Navigation.h>
#include <ui/StateContext.h>
#include <ui/Window.h>
#include <video/RenderContext.h>

static bool CanNavigateBack(TextureManager &texman, UI::Window *wnd, const UI::LayoutContext &lc, const UI::DataContext &dc)
{
	if (wnd)
	{
		if (auto navigationSink = wnd->GetNavigationSink(); navigationSink && navigationSink->CanNavigate(UI::Navigate::Back, lc, dc))
			return true;
		if (auto focusedChild = wnd->GetFocus())
		{
			auto childRect = wnd->GetChildRect(texman, lc, dc, *focusedChild);
			UI::LayoutContext childLC(*wnd, lc, *focusedChild, Size(childRect), dc);
			return CanNavigateBack(texman, focusedChild.get(), childLC, dc);
		}
	}
	return false;
}

UIInputRenderingController::UIInputRenderingController(AppWindow &appWindow,
                                                       TextureManager &textureManager,
                                                       UI::TimeStepManager &timeStepManager,
                                                       std::shared_ptr<UI::Window> desktop)
	: _appWindow(appWindow)
	, _inputContext(appWindow.GetInput(), appWindow.GetClipboard())
	, _textureManager(textureManager)
	, _timeStepManager(timeStepManager)
	, _desktop(desktop)
{
	_appWindow.SetInputSink(this);
}

UIInputRenderingController::~UIInputRenderingController()
{
	_appWindow.SetInputSink(nullptr);
}

void UIInputRenderingController::TimeStep(float dt)
{
	_inputContext.ReadInput();
	_timeStepManager.TimeStep(_desktop, _inputContext, dt);
}

bool UIInputRenderingController::OnChar(unsigned int codepoint)
{
	UI::DataContext dataContext;
	return _inputContext.ProcessText(
		_textureManager,
		_desktop,
		UI::LayoutContext(1.f, _appWindow.GetLayoutScale(), _appWindow.GetPixelSize(), _desktop->GetEnabled(dataContext)),
		dataContext,
		codepoint);
}

bool UIInputRenderingController::OnKey(UI::Key key, UI::Msg action)
{
	UI::DataContext dataContext;
	return _inputContext.ProcessKeys(
		_textureManager,
		_desktop,
		UI::LayoutContext(1.f, _appWindow.GetLayoutScale(), _appWindow.GetPixelSize(), _desktop->GetEnabled(dataContext)),
		dataContext,
		action,
		key,
		_timeStepManager.GetTime());
}

bool UIInputRenderingController::OnPointer(UI::PointerType pointerType, UI::Msg action, vec2d pxPointerPos, vec2d pxPointerOffset, int buttons, unsigned int pointerID)
{
	UI::DataContext dataContext;
	return _inputContext.ProcessPointer(
		_textureManager,
		_desktop,
		UI::LayoutContext(1.f, _appWindow.GetLayoutScale(), _appWindow.GetPixelSize(), _desktop->GetEnabled(dataContext)),
		dataContext,
		pxPointerPos,
		pxPointerOffset,
		action,
		buttons,
		pointerType,
		pointerID);
}

bool UIInputRenderingController::OnSystemNavigationBack()
{
	UI::DataContext dataContext;
	return _inputContext.ProcessSystemNavigationBack(
		_textureManager,
		_desktop,
		UI::LayoutContext(1.f, _appWindow.GetLayoutScale(), _appWindow.GetPixelSize(), _desktop->GetEnabled(dataContext)),
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

void UIInputRenderingController::OnRefresh()
{
	// not needed for single view
	_appWindow.MakeCurrent();

	vec2d pxWindowSize = _appWindow.GetPixelSize();
	IRender &render = _appWindow.GetRender();

	auto displayWidth = static_cast<unsigned int>(pxWindowSize.x);
	auto displayHeight = static_cast<unsigned int>(pxWindowSize.y);
	render.Begin(displayWidth, displayHeight, DOFromDegrees(_appWindow.GetDisplayRotation()));
	render.SetViewport({ 0, 0, (int)displayWidth, (int)displayHeight });
	RenderContext rc(_textureManager, render, displayWidth, displayHeight);

	UI::DataContext dataContext;
	UI::LayoutContext layoutContext(1.f, _appWindow.GetLayoutScale(), pxWindowSize, _desktop->GetEnabled(dataContext));
	UI::RenderSettings rs{ _inputContext, rc, _textureManager, _timeStepManager.GetTime() };

	UI::RenderUIRoot(*_desktop, rs, layoutContext, dataContext, UI::StateContext());

	bool hoverTextSink = false;
	for (auto &wnd : rs.hoverPath)
	{
		if (wnd->GetTextSink())
		{
			hoverTextSink = true;
			break;
		}
	}

	_appWindow.SetCanNavigateBack(CanNavigateBack(_textureManager, _desktop.get(), layoutContext, dataContext));

	MouseCursor mouseCursor = hoverTextSink ? MouseCursor::IBeam : MouseCursor::Arrow;
	if (_mouseCursor != mouseCursor)
	{
		_mouseCursor = mouseCursor;
		_appWindow.SetMouseCursor(mouseCursor);
	}

#ifndef NDEBUG
	for (auto &id2pos : rs.ic.GetLastPointerLocation())
	{
		FRECT dst = { id2pos.second.x - 4, id2pos.second.y - 4, id2pos.second.x + 4, id2pos.second.y + 4 };
		rs.rc.DrawSprite(dst, 0U, 0xffffffff, 0U);
	}
#endif

	render.End();
	_appWindow.Present();
}
