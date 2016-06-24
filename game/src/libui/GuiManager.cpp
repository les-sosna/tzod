#include "inc/ui/InputContext.h"
#include "inc/ui/UIInput.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/LayoutContext.h"
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

struct RenderSettings
{
	InputContext &ic;
	DrawingContext &dc;
	TextureManager &texman;
	bool topMostPass;
	std::vector<std::shared_ptr<Window>> hoverPath;
};

static void DrawWindowRecursive(
	RenderSettings &renderSettings,
	const Window &wnd,
	vec2d size,
	bool enabled,
	bool insideTopMost,
	bool onHoverPath,
	unsigned int depth = 0)
{
	if (insideTopMost == renderSettings.topMostPass)
	{
		LayoutContext lc(size, enabled, onHoverPath);
		wnd.Draw(lc, renderSettings.ic, renderSettings.dc, renderSettings.texman);
	}

	// topmost windows escape parents' clip
	bool clipChildren = wnd.GetClipChildren() && (!renderSettings.topMostPass || insideTopMost);

	if (clipChildren)
	{
		RectRB clip;
		clip.left = 0;
		clip.top = 0;
		clip.right = static_cast<int>(size.x);
		clip.bottom = static_cast<int>(size.y);
		renderSettings.dc.PushClippingRect(clip);
	}

	for (auto &child : wnd.GetChildren())
	{
		if (child->GetVisible())
		{
			FRECT childRect = wnd.GetChildRect(size, *child);
			bool childEnabled = enabled && child->GetEnabled();
			bool childInsideTopMost = insideTopMost || child->GetTopMost();
			bool childOnHoverPath = onHoverPath && depth + 1 < renderSettings.hoverPath.size() &&
				renderSettings.hoverPath[renderSettings.hoverPath.size() - depth - 2] == child;
			unsigned int childDepth = depth + 1;

			// early skip topmost window and all its children
			if (!childInsideTopMost || renderSettings.topMostPass)
			{
				vec2d offset(childRect.left, childRect.top);
				renderSettings.dc.PushTransform(offset);
				renderSettings.ic.PushTransform(offset, wnd.GetFocus() == child);

				DrawWindowRecursive(
					renderSettings,
					*child,
					Size(childRect),
					childEnabled,
					childInsideTopMost,
					childOnHoverPath,
					childDepth);

				renderSettings.ic.PopTransform();
				renderSettings.dc.PopTransform();
			}
		}
	}

	if (clipChildren)
		renderSettings.dc.PopClippingRect();
}

void LayoutManager::Render(vec2d size, DrawingContext &dc) const
{
	RenderSettings rs{ _inputContext, dc, _texman };

	// Find pointer sink path for hover
	// TODO: all pointers
	if (auto capturePath = _inputContext.GetCapturePath(0))
	{
		rs.hoverPath = *capturePath;
	}
	else
	{
		for (bool topMostPass : {true, false})
		{
			PointerSinkSearch search{ topMostPass };
			if (FindPointerSink(search, _desktop, size, _inputContext.GetMousePos(), _desktop->GetTopMost()))
			{
				rs.hoverPath = std::move(search.outSinkPath);
				break;
			}
		}
	}

	dc.SetMode(RM_INTERFACE);

	rs.ic.PushTransform(vec2d(), _inputContext.GetMainWindowActive());
	for (bool topMostPass : {false, true})
	{
		rs.topMostPass = topMostPass;
		DrawWindowRecursive(
			rs,
			*_desktop,
			size,
			_desktop->GetEnabled(),
			_desktop->GetTopMost(),
			!rs.hoverPath.empty()
			);
	}
	rs.ic.PopTransform();

#ifndef NDEBUG
	for (auto &id2pos: _inputContext.GetLastPointerLocation())
	{
		FRECT dst = { id2pos.second.x-4, id2pos.second.y-4, id2pos.second.x+4, id2pos.second.y+4 };
		dc.DrawSprite(dst, 0U, 0xffffffff, 0U);
	}
#endif
}
