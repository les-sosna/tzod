#include "inc/ui/InputContext.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Window.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

using namespace UI;

LayoutManager::LayoutManager(TextureManager &texman, InputContext &ic)
	: _texman(texman)
	, _inputContext(ic)
	, _timestep()
	, _tsCurrent(_timestep.end())
	, _tsDeleteCurrent(false)
{
}

LayoutManager::~LayoutManager()
{
}

std::list<Window*>::iterator LayoutManager::TimeStepRegister(Window* wnd)
{
	_timestep.push_front(wnd);
	return _timestep.begin();
}

void LayoutManager::TimeStepUnregister(std::list<Window*>::iterator it)
{
	if( _tsCurrent == it )
	{
		assert(!_tsDeleteCurrent);
		_tsDeleteCurrent = true;
	}
	else
	{
		_timestep.erase(it);
	}
}

void LayoutManager::TimeStep(float dt)
{
	assert(_tsCurrent == _timestep.end());
	assert(!_tsDeleteCurrent);

	_time += dt;

	for( _tsCurrent = _timestep.begin(); _tsCurrent != _timestep.end(); )
	{
		(*_tsCurrent)->OnTimeStep(*this, dt);
		if (_tsDeleteCurrent)
		{
			_tsDeleteCurrent = false;
			_tsCurrent = _timestep.erase(_tsCurrent);
		}
		else
		{
			++_tsCurrent;
		}
	}
}

static void DrawWindowRecursive(
	bool focused,
	bool enabled,
	const FRECT &rect,
	const Window &wnd,
	InputContext &ic,
	DrawingContext &dc,
	TextureManager &texman,
	bool topMostPass,
	bool insideTopMost)
{
	if (insideTopMost && !topMostPass)
		return; // early skip topmost window and all its children

	vec2d offset(rect.left, rect.top);
	dc.PushTransform(offset);
	ic.PushTransform(offset);

	vec2d size = Size(rect);

	if (insideTopMost == topMostPass)
		wnd.Draw(focused, enabled, size, ic, dc, texman);

	// topmost windows escape parents' clip
	bool clipChildren = wnd.GetClipChildren() && (!topMostPass || insideTopMost);

	if (clipChildren)
	{
		RectRB clip;
		clip.left = 0;
		clip.top = 0;
		clip.right = static_cast<int>(size.x);
		clip.bottom = static_cast<int>(size.y);
		dc.PushClippingRect(clip);
	}

	for (auto &child : wnd.GetChildren())
	{
		if (child->GetVisible())
		{
			FRECT childRect = wnd.GetChildRect(Size(rect), *child);
			bool childFocused = focused && (wnd.GetFocus() == child);
			bool childEnabled = enabled && wnd.GetEnabled();
			bool childInsideTopMost = insideTopMost || child->GetTopMost();
			DrawWindowRecursive(childFocused, childEnabled, childRect, *child, ic, dc, texman, topMostPass, childInsideTopMost);
		}
	}

	if (clipChildren)
		dc.PopClippingRect();

	ic.PopTransform();
	dc.PopTransform();
}

void LayoutManager::Render(FRECT rect, DrawingContext &dc) const
{
	dc.SetMode(RM_INTERFACE);

	for (int pass = 0; pass != 2; pass++)
	{
		bool topMostPass = !!pass;
		DrawWindowRecursive(
			_inputContext.GetMainWindowActive(),
			_desktop->GetEnabled(),
			rect,
			*_desktop,
			_inputContext,
			dc,
			_texman,
			topMostPass,
			_desktop->GetTopMost());
	}

#ifndef NDEBUG
	for (auto &id2pos: _inputContext.GetLastPointerLocation())
	{
		FRECT dst = { id2pos.second.x-4, id2pos.second.y-4, id2pos.second.x+4, id2pos.second.y+4 };
		dc.DrawSprite(&dst, 0U, 0xffffffff, 0U);
	}
#endif
}
