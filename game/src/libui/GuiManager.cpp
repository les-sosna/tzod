#include "inc/ui/InputContext.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/StateContext.h"
#include "inc/ui/Window.h"
#include "inc/ui/WindowIterator.h"
#include <plat/Input.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

using namespace UI;

TimeStepManager::TimeStepManager()
	: _tsCurrent(_timestep.end())
	, _tsDeleteCurrent(false)
{
}

std::list<TimeStepping*>::iterator TimeStepManager::TimeStepRegister(TimeStepping* wnd)
{
	_timestep.push_front(wnd);
	return _timestep.begin();
}

void TimeStepManager::TimeStepUnregister(std::list<TimeStepping*>::iterator it)
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

void TimeStepManager::TimeStep(std::shared_ptr<Window> desktop, const InputContext &ic, float dt)
{
	assert(_tsCurrent == _timestep.end());
	assert(!_tsDeleteCurrent);

	_time += dt;

	for( _tsCurrent = _timestep.begin(); _tsCurrent != _timestep.end(); )
	{
		bool focused = false;
		for (auto wnd = desktop; wnd; wnd = wnd->GetFocus())
		{
			if (*_tsCurrent == dynamic_cast<TimeStepping*>(wnd.get()))
			{
				focused = true;
				break;
			}
		}

		(*_tsCurrent)->OnTimeStep(ic.GetInput(), focused, dt);

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

//////////////////////////////////////////////////////////////

static void DrawWindowRecursive(
	const RenderSettings &renderSettings,
	const Window &wnd,
	const LayoutContext &lc,
	const StateContext &sc,
	const DataContext &dc,
	bool insideTopMost,
	unsigned int depth = 0)
{
	bool hovered = depth < renderSettings.hoverPath.size() &&
		renderSettings.hoverPath[renderSettings.hoverPath.size() - 1 - depth].get() == &wnd;
	if (insideTopMost == renderSettings.topMostPass)
		wnd.Draw(dc, sc, lc, renderSettings.ic, renderSettings.rc, renderSettings.texman, renderSettings.time, hovered);

	StateContext childCS;
	auto stateGen = wnd.GetStateGen();
	if (stateGen)
	{
		childCS = sc;
		stateGen->PushState(childCS, lc, renderSettings.ic, hovered);
	}

	// topmost windows escape parents' clip
	bool clipChildren = wnd.GetClipChildren() && (!renderSettings.topMostPass || insideTopMost);

	vec2d pxSize = lc.GetPixelSize();

	if (clipChildren)
	{
		RectRB clip;
		clip.left = 0;
		clip.top = 0;
		clip.right = static_cast<int>(pxSize.x);
		clip.bottom = static_cast<int>(pxSize.y);
		renderSettings.rc.PushClippingRect(clip);
	}

	FRECT visibleRegion = renderSettings.rc.GetVisibleRegion();

	unsigned int childDepth = depth + 1;
	for (auto child : wnd)
	{
		if (child->GetVisible())
		{
			// early skip topmost window and all its children
			bool childInsideTopMost = insideTopMost || child->GetTopMost();
			if (!childInsideTopMost || renderSettings.topMostPass)
			{
				auto childRect = wnd.GetChildRect(renderSettings.texman, lc, dc, *child);
				bool canDrawOutside = child->GetChildrenCount() > 0 && !child->GetClipChildren();

				if (canDrawOutside || RectIntersect(visibleRegion, childRect))
				{
					vec2d childOffset = Offset(childRect);
					LayoutContext childLC(renderSettings.ic, wnd, lc, *child, childOffset, Size(childRect));
					if (childLC.GetOpacityCombined() != 0)
					{
						renderSettings.rc.PushTransform(childOffset, childLC.GetOpacityCombined());

						DrawWindowRecursive(
							renderSettings,
							*child,
							childLC,
							stateGen ? childCS : sc,
							dc,
							childInsideTopMost,
							childDepth);

						renderSettings.rc.PopTransform();
					}
				}
			}
		}
	}

	if (clipChildren)
		renderSettings.rc.PopClippingRect();
}

void UI::RenderUIRoot(Window &desktop, RenderSettings &rs, const LayoutContext &lc, const DataContext &dc, const StateContext &sc)
{
	// Find pointer sink path for hover
	// TODO: all pointers
	if (auto capturePath = rs.ic.GetCapturePath(0))
	{
		rs.hoverPath = *capturePath;
	}
	else if (rs.ic.GetPointerType(0) != Plat::PointerType::Unknown)
	{
		for (bool topMostPass : {true, false})
		{
			AreaSinkSearch search{ rs.texman, rs.ic, dc, topMostPass };
			if (FindAreaSink<PointerSink>(search, desktop.shared_from_this(), lc, rs.ic.GetPointerPos(0, lc), desktop.GetTopMost()))
			{
				rs.hoverPath = std::move(search.outSinkPath);
				break;
			}
		}
	}

	rs.rc.SetMode(RM_INTERFACE);
	for (bool topMostPass : {false, true})
	{
		rs.topMostPass = topMostPass;
		DrawWindowRecursive(
			rs,
			desktop,
			lc,
			sc,
			dc,
			desktop.GetTopMost()
		);
	}
}
