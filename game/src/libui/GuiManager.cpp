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
	LayoutContext &lc;
	InputContext &ic;
	DrawingContext &dc;
	TextureManager &texman;
	bool topMostPass;
	std::vector<std::shared_ptr<Window>> hoverPath;
};

static void DrawWindowRecursive(
	RenderSettings &renderSettings,
	const Window &wnd,
	bool insideTopMost,
	unsigned int depth = 0)
{
	if (insideTopMost == renderSettings.topMostPass)
		wnd.Draw(renderSettings.lc, renderSettings.ic, renderSettings.dc, renderSettings.texman);

	// topmost windows escape parents' clip
	bool clipChildren = wnd.GetClipChildren() && (!renderSettings.topMostPass || insideTopMost);

	vec2d pxSize = renderSettings.lc.GetPixelSize();

	if (clipChildren)
	{
		RectRB clip;
		clip.left = 0;
		clip.top = 0;
		clip.right = static_cast<int>(pxSize.x);
		clip.bottom = static_cast<int>(pxSize.y);
		renderSettings.dc.PushClippingRect(clip);
	}

	unsigned int childDepth = depth + 1;
	for (auto &child : wnd.GetChildren())
	{
		if (child->GetVisible())
		{
			// early skip topmost window and all its children
			bool childInsideTopMost = insideTopMost || child->GetTopMost();
			if (!childInsideTopMost || renderSettings.topMostPass)
			{
				FRECT childRect = wnd.GetChildRect(pxSize, renderSettings.lc.GetScale(), *child);
				bool childFocused = wnd.GetFocus() == child;
				bool childOnHoverPath = childDepth < renderSettings.hoverPath.size() &&
					renderSettings.hoverPath[renderSettings.hoverPath.size() - 1 - childDepth] == child;

				vec2d childOffset{ childRect.left, childRect.top };

				renderSettings.dc.PushTransform(childOffset);
				renderSettings.ic.PushTransform(childOffset, childFocused, childOnHoverPath);
				renderSettings.lc.PushTransform(Size(childRect), child->GetEnabled());

				DrawWindowRecursive(
					renderSettings,
					*child,
					childInsideTopMost,
					childDepth);

				renderSettings.lc.PopTransform();
				renderSettings.ic.PopTransform();
				renderSettings.dc.PopTransform();
			}
		}
	}

	if (clipChildren)
		renderSettings.dc.PopClippingRect();
}

void LayoutManager::Render(float layoutScale, vec2d size, DrawingContext &dc) const
{
	LayoutContext layoutContext(layoutScale, size, _desktop->GetEnabled());
	RenderSettings rs{ layoutContext, _inputContext, dc, _texman };

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
			AreaSinkSearch search{ layoutContext.GetScale(), topMostPass };
			if (FindAreaSink<PointerSink>(search, _desktop, size, _inputContext.GetMousePos(), _desktop->GetTopMost()))
			{
				rs.hoverPath = std::move(search.outSinkPath);
				break;
			}
		}
	}

	dc.SetMode(RM_INTERFACE);

	rs.ic.PushTransform(vec2d{}, _inputContext.GetMainWindowActive(), !rs.hoverPath.empty());
	for (bool topMostPass : {false, true})
	{
		rs.topMostPass = topMostPass;
		DrawWindowRecursive(
			rs,
			*_desktop,
			_desktop->GetTopMost()
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
