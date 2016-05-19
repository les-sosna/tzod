// GuiManager.cpp

#include "inc/ui/GuiManager.h"
#include "inc/ui/Window.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

namespace UI
{

LayoutManager::LayoutManager(IInput &input, IClipboard &clipboard, TextureManager &texman)
  : _input(input)
  , _clipboard(clipboard)
  , _texman(texman)
  , _timestep()
  , _tsCurrent(_timestep.end())
  , _tsDeleteCurrent(false)
  , _captureCountSystem(0)
  , _isAppActive(false)
#ifndef NDEBUG
  , _dbgFocusIsChanging(false)
  , _lastPointerLocation()
#endif
{
}

LayoutManager::~LayoutManager()
{
}

std::shared_ptr<Window> LayoutManager::GetCapture(unsigned int pointerID) const
{
	auto it = _pointerCaptures.find(pointerID);
	return _pointerCaptures.end() != it ? it->second.captureWnd.lock() : nullptr;
}

bool LayoutManager::HasCapturedPointers(Window *wnd) const
{
	for (auto &capture: _pointerCaptures)
	{
		if (capture.second.captureWnd.lock().get() == wnd)
		{
			return true;
		}
	}
	return false;
}

Window* LayoutManager::GetDesktop() const
{
	return _desktop.get();
}

void LayoutManager::SetCapture(unsigned int pointerID, std::shared_ptr<Window> wnd)
{
	if( wnd )
	{
		if( !_pointerCaptures[pointerID].captureWnd.expired() )
		{
			assert(_pointerCaptures[pointerID].captureWnd.lock() == wnd);
			assert(_pointerCaptures[pointerID].captureCount != 0);
		}
		else
		{
			assert(0 == _pointerCaptures[pointerID].captureCount);
			_pointerCaptures[pointerID].captureWnd = wnd;
		}
		_pointerCaptures[pointerID].captureCount++;
	}
	else
	{
		assert(!_pointerCaptures[pointerID].captureWnd.expired());
		assert(0 != _pointerCaptures[pointerID].captureCount);
		if( 0 == --_pointerCaptures[pointerID].captureCount )
		{
			_pointerCaptures[pointerID].captureWnd.reset();
		}
	}
}

void LayoutManager::ResetWindow(Window &wnd)
{
	if( _hotTrackWnd.lock().get() == &wnd )
	{
		wnd.OnMouseLeave();
		_hotTrackWnd.reset();
	}

	_pointerCaptures.clear();
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
		(*_tsCurrent)->OnTimeStep(dt);
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

bool LayoutManager::ProcessPointerInternal(
	vec2d size,
	std::shared_ptr<Window> wnd,
	float x,
	float y,
	float z,
	Msg msg,
	int buttons,
	PointerType pointerType,
	unsigned int pointerID,
	bool topMostPass,
	bool insideTopMost)
{
	insideTopMost |= wnd->GetTopMost();

	if (!wnd->GetEnabled() || !wnd->GetVisible() || (insideTopMost && !topMostPass))
		return false;

	bool pointerInside = (x >= 0 && x < size.x && y >= 0 && y < size.y);

	if( (pointerInside || !wnd->GetClipChildren()) && GetCapture(pointerID) != wnd )
	{
		// route message to each child in reverse order until someone process it
		for (auto it = wnd->_children.rbegin(); it != wnd->_children.rend(); ++it)
		{
			auto &child = *it;
			FRECT rect = wnd->GetChildRect(size, *child);
			if( ProcessPointerInternal(Size(rect),
				                       child,
				                       x - rect.left,
				                       y - rect.top,
				                       z,
				                       msg,
				                       buttons,
				                       pointerType,
				                       pointerID,
				                       topMostPass,
				                       insideTopMost) )
			{
				switch( msg )
				{
				case Msg::PointerDown:
				case Msg::TAP:
					if (child->GetEnabledCombined() && child->GetVisibleCombined() && child->GetNeedsFocus())
					{
						wnd->SetFocus(child);
					}
				default:
					break;
				}

				return true;
			}
		}
	}

	if( insideTopMost == topMostPass && (pointerInside || GetCapture(pointerID) == wnd) )
	{
		// window is captured or the pointer is inside the window

		bool msgProcessed = false;
		switch( msg )
		{
			case Msg::PointerDown:
				msgProcessed = wnd->OnPointerDown(x, y, buttons, pointerType, pointerID);
				break;
			case Msg::PointerUp:
			case Msg::PointerCancel:
				msgProcessed = wnd->OnPointerUp(x, y, buttons, pointerType, pointerID);
				break;
			case Msg::PointerMove:
				msgProcessed = wnd->OnPointerMove(x, y, pointerType, pointerID);
				break;
			case Msg::MOUSEWHEEL:
				msgProcessed = wnd->OnMouseWheel(x, y, z);
				break;
			case Msg::TAP:
				msgProcessed = wnd->OnTap(x, y);
				break;
			default:
				assert(false);
		}

		if( msgProcessed )
		{
			if( wnd != _hotTrackWnd.lock() )
			{
				if( auto hotTrackWnd = _hotTrackWnd.lock() )
					hotTrackWnd->OnMouseLeave();
				if( wnd->GetVisibleCombined() && wnd->GetEnabledCombined() )
				{
					_hotTrackWnd = wnd;
					wnd->OnMouseEnter(x, y);
				}
			}
		}

		return msgProcessed;
	}

	return false;
}

static FRECT GetGlobalWindowRect(const vec2d &rootSize, const Window &wnd)
{
	if (Window *parent = wnd.GetParent())
	{
		FRECT parentRect = GetGlobalWindowRect(rootSize, *parent);
		FRECT childRect = parent->GetChildRect(Size(parentRect), wnd);
		childRect.left += parentRect.left;
		childRect.right += parentRect.left;
		childRect.top += parentRect.top;
		childRect.bottom += parentRect.top;
		return childRect;
	}
	else
	{
		return FRECT{ 0, 0, rootSize.x, rootSize.y };
	}
}

bool LayoutManager::ProcessPointer(vec2d size, float x, float y, float z, Msg msg, int button, PointerType pointerType, unsigned int pointerID)
{
#ifndef NDEBUG
    _lastPointerLocation[pointerID] = vec2d(x, y);
#endif

	if( auto captured = GetCapture(pointerID) )
	{
		FRECT capturetRect = GetGlobalWindowRect(size, *captured);
		vec2d capturedSize = Size(capturetRect);

		x -= capturetRect.left;
		y -= capturetRect.top;

		if (ProcessPointerInternal(capturedSize, captured, x, y, z, msg, button, pointerType, pointerID, true) ||
			ProcessPointerInternal(capturedSize, captured, x, y, z, msg, button, pointerType, pointerID, false))
		{
			return true;
		}
	}
	else
	{
		// handle all children of the desktop recursively; offer to topmost windows first
		if (ProcessPointerInternal(size, _desktop, x, y, z, msg, button, pointerType, pointerID, true) ||
			ProcessPointerInternal(size, _desktop, x, y, z, msg, button, pointerType, pointerID, false))
		{
			return true;
		}
	}
	if( auto hotTrackWnd = _hotTrackWnd.lock() )
	{
		hotTrackWnd->OnMouseLeave();
		_hotTrackWnd.reset();
	}
	return false;
}

bool LayoutManager::ProcessKeyPressedRecursive(std::shared_ptr<Window> wnd, Key key)
{
	if (auto focus = wnd->GetFocus())
	{
		if (ProcessKeyPressedRecursive(std::move(focus), key))
		{
			return true;
		}
	}
	return wnd->OnKeyPressed(key);
}

bool LayoutManager::ProcessKeys(Msg msg, Key key)
{
	switch( msg )
	{
	case Msg::KEYUP:
		break;
	case Msg::KEYDOWN:
		return ProcessKeyPressedRecursive(_desktop, key);
	default:
		assert(false);
	}

	return false;
}

bool LayoutManager::ProcessCharRecursive(std::shared_ptr<Window> wnd, int c)
{
	if (auto focus = wnd->GetFocus())
	{
		if (ProcessCharRecursive(std::move(focus), c))
		{
			return true;
		}
	}
	return wnd->OnChar(c);
}

bool LayoutManager::ProcessText(int c)
{
	return ProcessCharRecursive(_desktop, c);
}

static void DrawWindowRecursive(bool focused, const FRECT &rect, const Window &wnd, DrawingContext &dc, TextureManager &texman, bool topMostPass, bool insideTopMost)
{
	insideTopMost |= wnd.GetTopMost();

	if (!wnd.GetVisible() || (insideTopMost && !topMostPass))
		return; // skip window and all its children

	dc.PushTransform(vec2d(rect.left, rect.top));

	vec2d size = Size(rect);

	if (insideTopMost == topMostPass)
		wnd.Draw(focused, size, dc, texman);

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

	for (auto &w : wnd.GetChildren())
	{
		FRECT childRect = wnd.GetChildRect(Size(rect), *w);
		bool childFocused = focused && (wnd.GetFocus() == w);
		DrawWindowRecursive(childFocused, childRect, *w, dc, texman, topMostPass, wnd.GetTopMost() || insideTopMost);
	}

	if (clipChildren)
		dc.PopClippingRect();

	dc.PopTransform();
}

void LayoutManager::Render(FRECT rect, DrawingContext &dc) const
{
	dc.SetMode(RM_INTERFACE);

	DrawWindowRecursive(true, rect, *_desktop, dc, GetTextureManager(), false, false);
	DrawWindowRecursive(true, rect, *_desktop, dc, GetTextureManager(), true, false);

#ifndef NDEBUG
	for (auto &id2pos: _lastPointerLocation)
	{
		FRECT dst = { id2pos.second.x-4, id2pos.second.y-4, id2pos.second.x+4, id2pos.second.y+4 };
		dc.DrawSprite(&dst, 0U, 0xffffffff, 0U);
	}
#endif
}

} // namespace UI
