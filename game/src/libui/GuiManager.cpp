// GuiManager.cpp

#include "inc/ui/GuiManager.h"
#include "inc/ui/Window.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

namespace UI
{

LayoutManager::LayoutManager(IInput &input, IClipboard &clipboard, TextureManager &texman, IWindowFactory &&desktopFactory)
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
	_desktop = desktopFactory.Create(*this);
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

void LayoutManager::AddTopMost(std::shared_ptr<Window> wnd, bool add)
{
	if( add )
	{
		_topmost.push_back(wnd.get());
	}
	else
	{
		for( auto it = _topmost.begin(); _topmost.end() != it; ++it )
		{
			if( *it == wnd.get())
			{
				_topmost.erase(it);
				break;
			}
		}
	}
}

bool LayoutManager::SetFocusWnd(const std::shared_ptr<Window> &wnd)
{
	assert(!_dbgFocusIsChanging);
	if( _focusWnd.lock() != wnd )
	{
		auto oldFocusWnd = _focusWnd.lock();

		// try setting new focus. it should not change _focusWnd
#ifndef NDEBUG
		_dbgFocusIsChanging = true;
#endif
		bool focusAccepted = wnd && wnd->GetEnabled() && wnd->GetVisibleCombined() && wnd->OnFocus(true);
#ifndef NDEBUG
		_dbgFocusIsChanging = false;
#endif
		if( !focusAccepted && wnd && oldFocusWnd )
		{
			for( auto w = wnd->GetParent(); w; w = w->GetParent() )
			{
				if( w == oldFocusWnd.get() )
				{
					// don't reset focus from parent
					return false;
				}
			}
		}

		// set new focus
		_focusWnd = focusAccepted ? wnd : nullptr;
		assert(_focusWnd.expired() || _focusWnd.lock()->GetEnabled() && _focusWnd.lock()->GetVisibleCombined());

		// reset old focus
		if( oldFocusWnd && oldFocusWnd != _focusWnd.lock() )
		{
			oldFocusWnd->OnFocus(false);
			if( oldFocusWnd->eventLostFocus )
				oldFocusWnd->eventLostFocus();
		}
	}
	return !_focusWnd.expired();
}

std::shared_ptr<Window> LayoutManager::GetFocusWnd() const
{
	assert(!_dbgFocusIsChanging);
	return _focusWnd.lock();
}

/*
bool LayoutManager::ResetFocus(Window* wnd)
{
	assert(wnd);
	assert(_focusWnd);

	if( wnd == _focusWnd )
	{
		//
		// search for first appropriate parent
		//

		Window *tmp = wnd;
		for( Window *w = wnd->GetParent(); w; w = w->GetParent() )
		{
			if( !w->GetVisibleCombined() || !w->GetEnabled() )
			{
				tmp = w->GetParent();
			}
		}

		while( tmp )
		{
			if( wnd != tmp && SetFocusWnd(tmp) )
			{
				break;
			}

			Window *r;

			// try to pass focus to next siblings
			for( r = tmp->GetNextSibling(); r; r = r->GetNextSibling() )
			{
				if( r->GetVisibleCombined() && r->GetEnabled() )
				{
					if( SetFocusWnd(r) ) break;
				}
			}
			if( r ) break;

			// try to pass focus to previous siblings
			for( r = tmp->GetPrevSibling(); r; r = r->GetPrevSibling() )
			{
				if( r->GetVisibleCombined() && r->GetEnabled() )
				{
					if( SetFocusWnd(r) ) break;
				}
			}
			if( r ) break;

			// and finally try to pass focus to the parent and its siblings
			tmp = tmp->GetParent();
			assert(!tmp || (tmp->GetVisibleCombined() && tmp->GetEnabled()));
		}
		if( !tmp )
		{
			SetFocusWnd(nullptr);
		}
		assert(wnd != _focusWnd);
		return true;
	}

	for( Window *w = wnd->GetFirstChild(); w; w = w->GetNextSibling() )
	{
		if( ResetFocus(w) )
		{
			return true;
		}
	}

	return false;
}
*/
void LayoutManager::ResetWindow(Window &wnd)
{
	if( GetFocusWnd().get() == &wnd )
		SetFocusWnd(nullptr);

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

bool LayoutManager::ProcessPointerInternal(std::shared_ptr<Window> wnd, float x, float y, float z, Msg msg, int buttons, PointerType pointerType, unsigned int pointerID)
{
	bool bMouseInside = (x >= 0 && x < wnd->GetWidth() && y >= 0 && y < wnd->GetHeight());

	if( GetCapture(pointerID) != wnd )
	{
		// route message to each child until someone process it
		if( bMouseInside || !wnd->GetClipChildren() )
		{
			for (auto it = wnd->_children.rbegin(); it != wnd->_children.rend(); ++it)
			{
				// do not dispatch messages to disabled or invisible window.
				// topmost windows are processed separately
				auto &w = *it;
				if( w->GetEnabled() && w->GetVisibleCombined() && !w->GetTopMost() &&
					ProcessPointerInternal(w, x - w->GetX(), y - w->GetY(), z, msg, buttons, pointerType, pointerID) )
				{
					return true;
				}
			}
		}
	}

	if( bMouseInside || GetCapture(pointerID) == wnd )
	{
		//
		// window is captured or mouse pointer is inside the window
		//

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
			switch( msg )
			{
			case Msg::PointerDown:
			case Msg::TAP:
				SetFocusWnd(wnd);
			default:
				break;
			}

			if( wnd != _hotTrackWnd.lock() )
			{
				if( auto hotTrackWnd = _hotTrackWnd.lock() )
					hotTrackWnd->OnMouseLeave();
				if( wnd->GetVisibleCombined() && wnd->GetEnabled() )
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

bool LayoutManager::ProcessPointer(float x, float y, float z, Msg msg, int button, PointerType pointerType, unsigned int pointerID)
{
#ifndef NDEBUG
    _lastPointerLocation[pointerID] = vec2d(x, y);
#endif

	if( auto captured = GetCapture(pointerID) )
	{
		// calc relative mouse position and route message to captured window
		for( auto wnd = captured.get(); _desktop.get() != wnd; wnd = wnd->GetParent() )
		{
			assert(wnd);
			x -= wnd->GetX();
			y -= wnd->GetY();
		}
		if( ProcessPointerInternal(captured, x, y, z, msg, button, pointerType, pointerID) )
			return true;
	}
	else
	{
		// first try to pass messages to one of topmost windows
		for( auto it = _topmost.rbegin(); _topmost.rend() != it; ++it )
		{
			// do not dispatch messages to disabled or invisible window
			if( (*it)->GetEnabled() && (*it)->GetVisibleCombined() )
			{
				// calculate absolute coordinates of the window
				float x_ = _desktop->GetX();
				float y_ = _desktop->GetY();
				for( auto wnd = *it; _desktop.get() != wnd; wnd = wnd->GetParent() )
				{
					assert(wnd);
					x_ += wnd->GetX();
					y_ += wnd->GetY();
				}
				if( ProcessPointerInternal((*it)->shared_from_this(), x - x_, y - y_, z, msg, button, pointerType, pointerID) )
					return true;
			}
		}
		// then handle all children of the desktop recursively
		if( ProcessPointerInternal(_desktop, x, y, z, msg, button, pointerType, pointerID) )
			return true;
	}
	if( auto hotTrackWnd = _hotTrackWnd.lock() )
	{
		hotTrackWnd->OnMouseLeave();
		_hotTrackWnd.reset();
	}
	return false;
}

bool LayoutManager::ProcessKeys(Msg msg, UI::Key key)
{
	switch( msg )
	{
	case Msg::KEYUP:
		break;
	case Msg::KEYDOWN:
		if( auto wnd = GetFocusWnd().get() )
		{
			while( wnd )
			{
				if( wnd->OnKeyPressed(key) )
				{
					return true;
				}
				wnd = wnd->GetParent();
			}
		}
		else
		{
			GetDesktop()->OnKeyPressed(key);
		}
		break;
	default:
		assert(false);
	}

	return false;
}

bool LayoutManager::ProcessText(int c)
{
	if( auto wnd = GetFocusWnd().get() )
	{
		while( wnd )
		{
			if( wnd->OnChar(c) )
			{
				return true;
			}
			wnd = wnd->GetParent();
		}
	}
	else
	{
		GetDesktop()->OnChar(c);
	}
	return false;
}

static void DrawWindowRecursive(const Window &wnd, DrawingContext &dc)
{
	dc.PushTransform(vec2d(wnd.GetX(), wnd.GetY()));

	wnd.Draw(dc);

	if (wnd.GetClipChildren())
	{
		RectRB clip;
		clip.left = 0;
		clip.top = 0;
		clip.right = (int)wnd.GetWidth();
		clip.bottom = (int)wnd.GetHeight();
		dc.PushClippingRect(clip);
	}

	// draw children recursive
	for (auto &w: wnd.GetChildren())
	{
		// skip topmost windows; they are drawn separately
		if (!w->GetTopMost() && w->GetVisible())
		{
			DrawWindowRecursive(*w, dc);
		}
	}

	if (wnd.GetClipChildren())
	{
		dc.PopClippingRect();
	}

	dc.PopTransform();
}

void LayoutManager::Render(DrawingContext &dc) const
{
	dc.SetMode(RM_INTERFACE);

	// draw desktop and all its children
	if (_desktop->GetVisible())
	{
		DrawWindowRecursive(*_desktop, dc);
	}

	// draw top-most windows
	for( const Window *w: _topmost )
	{
		if( w->GetVisibleCombined() )
		{
			float x = _desktop->GetX();
			float y = _desktop->GetY();
			for( auto wnd = w->GetParent(); _desktop.get() != wnd; wnd = wnd->GetParent() )
			{
				assert(wnd);
				x += wnd->GetX();
				y += wnd->GetY();
			}
			dc.PushTransform(vec2d(x, y));
			DrawWindowRecursive(*w, dc);
			dc.PopTransform();
		}
	}

#ifndef NDEBUG
	for (auto &id2pos: _lastPointerLocation)
	{
		FRECT dst = { id2pos.second.x-4, id2pos.second.y-4, id2pos.second.x+4, id2pos.second.y+4 };
		dc.DrawSprite(&dst, 0U, 0xffffffff, 0U);
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
