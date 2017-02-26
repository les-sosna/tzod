#include "inc/ui/InputContext.h"
#include "inc/ui/UIInput.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/StateContext.h"
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
	RenderSettings &renderSettings,
	const Window &wnd,
	const LayoutContext &lc,
	const StateContext &sc,
	bool insideTopMost,
	unsigned int depth = 0)
{
	if (insideTopMost == renderSettings.topMostPass)
		wnd.Draw(sc, lc, renderSettings.ic, renderSettings.dc, renderSettings.texman);

	StateContext childCS;
	auto stateGen = wnd.GetStateGen();
	if (stateGen)
	{
		childCS = sc;
		stateGen->PushState(childCS, lc, renderSettings.ic);
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
				auto childRect = wnd.GetChildRect(renderSettings.texman, lc, sc, *child);
				LayoutContext childLC(wnd, lc, *child, childRect, stateGen ? childCS : sc);
				if (childLC.GetOpacityCombined() != 0)
				{
					bool childFocused = wnd.GetFocus() == child;
					bool childOnHoverPath = childDepth < renderSettings.hoverPath.size() &&
						renderSettings.hoverPath[renderSettings.hoverPath.size() - 1 - childDepth] == child;

					vec2d childOffset = Offset(childRect);
					renderSettings.dc.PushTransform(childOffset, childLC.GetOpacityCombined());
					renderSettings.ic.PushInputTransform(childOffset, childFocused, childOnHoverPath);

					DrawWindowRecursive(
						renderSettings,
						*child,
						childLC,
						stateGen ? childCS : sc,
						childInsideTopMost,
						childDepth);

					renderSettings.ic.PopInputTransform();
					renderSettings.dc.PopTransform();
				}
			}
		}
	}

	if (clipChildren)
		renderSettings.dc.PopClippingRect();
}

void UI::RenderUIRoot(Window &desktop, RenderSettings &rs, const LayoutContext &lc, const StateContext &sc)
{
	// Find pointer sink path for hover
	// TODO: all pointers
	if (auto capturePath = rs.ic.GetCapturePath(0))
	{
		rs.hoverPath = *capturePath;
	}
	else
	{
		for (bool topMostPass : {true, false})
		{
			AreaSinkSearch search{ rs.texman, rs.ic.GetMousePos(), topMostPass };
			if (FindAreaSink<PointerSink>(search, desktop.shared_from_this(), lc, sc, rs.ic, desktop.GetTopMost()))
			{
				rs.hoverPath = std::move(search.outSinkPath);
				break;
			}
		}
	}

	rs.dc.SetMode(RM_INTERFACE);

	rs.ic.PushInputTransform(vec2d{}, rs.ic.GetMainWindowActive(), !rs.hoverPath.empty());
	for (bool topMostPass : {false, true})
	{
		rs.topMostPass = topMostPass;
		DrawWindowRecursive(
			rs,
			desktop,
			lc,
			sc,
			desktop.GetTopMost()
		);
	}
	rs.ic.PopInputTransform();
}
