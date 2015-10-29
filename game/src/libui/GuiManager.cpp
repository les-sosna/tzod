// GuiManager.cpp

#include "inc/ui/GuiManager.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

namespace UI
{

LayoutManager::LayoutManager(IInput &input, IClipboard &clipboard, TextureManager &texman, IWindowFactory &&desktopFactory)
  : _input(input)
  , _clipboard(clipboard)
  , _texman(texman)
  , _tsCurrent(_timestep.end())
  , _tsDeleteCurrent(false)
  , _captureCountSystem(0)
  , _captureCount(0)
  , _focusWnd(nullptr)
  , _hotTrackWnd(nullptr)
  , _captureWnd(nullptr)
  , _desktop(nullptr)
  , _isAppActive(false)
#ifndef NDEBUG
  , _dbgFocusIsChanging(false)
#endif
{
	_desktop.Set(desktopFactory.Create(this));
}

LayoutManager::~LayoutManager()
{
	_desktop->Destroy();
	assert(_topmost.empty());
}

Window* LayoutManager::GetCapture() const
{
	return _captureWnd.Get();
}

Window* LayoutManager::GetDesktop() const
{
	return _desktop.Get();
}

void LayoutManager::SetCapture(Window* wnd)
{
	if( wnd )
	{
		if( _captureWnd.Get() )
		{
			assert(_captureWnd.Get() == wnd);
			assert(_captureCount != 0);
		}
		else
		{
			assert(0 == _captureCount);
			_captureWnd.Set(wnd);
		}
		_captureCount++;
	}
	else
	{
		assert(_captureWnd.Get());
		assert(0 != _captureCount);
		if( 0 == --_captureCount )
		{
			_captureWnd.Set(nullptr);
		}
	}
}

void LayoutManager::AddTopMost(Window* wnd, bool add)
{
	if( add )
	{
		_topmost.push_back(wnd);
	}
	else
	{
		for( auto it = _topmost.begin(); _topmost.end() != it; ++it )
		{
			if( *it == wnd )
			{
				_topmost.erase(it);
				break;
			}
		}
	}
}

bool LayoutManager::SetFocusWnd(Window* wnd)
{
	assert(!_dbgFocusIsChanging);
	if( _focusWnd.Get() != wnd )
	{
		WindowWeakPtr wp(wnd);
		WindowWeakPtr oldFocusWnd(_focusWnd.Get());

		// try setting new focus. it should not change _focusWnd
#ifndef NDEBUG
		_dbgFocusIsChanging = true;
#endif
		bool focusAccepted = wnd && wnd->GetEnabled() && wnd->GetVisible() && wnd->OnFocus(true);
#ifndef NDEBUG
		_dbgFocusIsChanging = false;
#endif
		if( !focusAccepted && wp.Get() && oldFocusWnd.Get() )
		{
			for( Window *w = wp.Get()->GetParent(); w; w = w->GetParent() )
			{
				if( w == oldFocusWnd.Get() )
				{
					// don't reset focus from parent
					return false;
				}
			}
		}

		// set new focus
		_focusWnd.Set(focusAccepted ? wp.Get() : nullptr);
		assert(!_focusWnd.Get() || _focusWnd->GetEnabled() && _focusWnd->GetVisible());

		// reset old focus
		if( oldFocusWnd.Get() && oldFocusWnd.Get() != _focusWnd.Get() )
		{
			oldFocusWnd->OnFocus(false); // _focusWnd may be destroyed here
			if( oldFocusWnd.Get() && oldFocusWnd->eventLostFocus )
				oldFocusWnd->eventLostFocus();
		}
	}
	return nullptr != _focusWnd.Get();
}

Window* LayoutManager::GetFocusWnd() const
{
	assert(!_dbgFocusIsChanging);
	return _focusWnd.Get();
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
			if( !w->GetVisible() || !w->GetEnabled() )
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
				if( r->GetVisible() && r->GetEnabled() )
				{
					if( SetFocusWnd(r) ) break;
				}
			}
			if( r ) break;

			// try to pass focus to previous siblings
			for( r = tmp->GetPrevSibling(); r; r = r->GetPrevSibling() )
			{
				if( r->GetVisible() && r->GetEnabled() )
				{
					if( SetFocusWnd(r) ) break;
				}
			}
			if( r ) break;

			// and finally try to pass focus to the parent and its siblings
			tmp = tmp->GetParent();
			assert(!tmp || (tmp->GetVisible() && tmp->GetEnabled()));
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
void LayoutManager::ResetWindow(Window* wnd)
{
	assert(wnd);

	if( GetFocusWnd() == wnd )
		SetFocusWnd(nullptr);

	if( _hotTrackWnd.Get() == wnd )
	{
		_hotTrackWnd->OnMouseLeave();
		_hotTrackWnd.Set(nullptr);
	}

	if( _captureWnd.Get() == wnd )
	{
		_captureWnd.Set(nullptr);
		_captureCount = 0;
	}
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

bool LayoutManager::ProcessMouseInternal(Window* wnd, float x, float y, float z, Msg msg)
{
	bool bMouseInside = (x >= 0 && x < wnd->GetWidth() && y >= 0 && y < wnd->GetHeight());

	if( GetCapture() != wnd )
	{
		// route message to each child until someone process it
		if( bMouseInside || !wnd->GetClipChildren() )
		{
			for( Window *w = wnd->GetLastChild(); w; w = w->GetPrevSibling() )
			{
#ifndef NDEBUG
				WindowWeakPtr wp(w);
#endif
				// do not dispatch messages to disabled or invisible window.
				// topmost windows are processed separately
				if( w->GetEnabled() && w->GetVisible() && !w->GetTopMost() &&
					ProcessMouseInternal(w, x - w->GetX(), y - w->GetY(), z, msg) )
				{
					return true;
				}
				assert(wp.Get());
			}
		}
	}

	if( bMouseInside || GetCapture() == wnd )
	{
		//
		// window is captured or mouse pointer is inside the window
		//

		WindowWeakPtr wp(wnd);

		bool msgProcessed = false;
		switch( msg )
		{
			case MSGLBUTTONDOWN:  msgProcessed = wnd->OnMouseDown(x,y, 1);  break;
			case MSGRBUTTONDOWN:  msgProcessed = wnd->OnMouseDown(x,y, 2);  break;
			case MSGMBUTTONDOWN:  msgProcessed = wnd->OnMouseDown(x,y, 3);  break;

			case MSGLBUTTONUP:    msgProcessed = wnd->OnMouseUp(x,y, 1);    break;
			case MSGRBUTTONUP:    msgProcessed = wnd->OnMouseUp(x,y, 2);    break;
			case MSGMBUTTONUP:    msgProcessed = wnd->OnMouseUp(x,y, 3);    break;

			case MSGMOUSEMOVE:    msgProcessed = wnd->OnMouseMove(x,y);     break;

			case MSGMOUSEWHEEL:   msgProcessed = wnd->OnMouseWheel(x,y,z);  break;
            default:
                assert(false);
		}
		// if window did not process the message, it should not destroy it self
		assert(msgProcessed || wp.Get());

		if( wp.Get() && msgProcessed )
		{
			switch( msg )
			{
			case MSGLBUTTONDOWN:
			case MSGRBUTTONDOWN:
			case MSGMBUTTONDOWN:
				SetFocusWnd(wnd); // may destroy wnd
            default:
                break;
			}

			if( wp.Get() && wnd != _hotTrackWnd.Get() )
			{
				if( _hotTrackWnd.Get() )
					_hotTrackWnd->OnMouseLeave(); // may destroy wnd
				if( wp.Get() && wnd->GetVisible() && wnd->GetEnabled() )
				{
					_hotTrackWnd.Set(wnd);
					_hotTrackWnd->OnMouseEnter(x, y);
				}
			}
		}

		return msgProcessed;
	}

	return false;
}

bool LayoutManager::ProcessMouse(float x, float y, float z, Msg msg)
{
	if( _captureWnd.Get() )
	{
		// calc relative mouse position and route message to captured window
		for( Window *wnd = _captureWnd.Get(); _desktop.Get() != wnd; wnd = wnd->GetParent() )
		{
			assert(wnd);
			x -= wnd->GetX();
			y -= wnd->GetY();
		}
		if( ProcessMouseInternal(_captureWnd.Get(), x, y, z, msg) )
			return true;
	}
	else
	{
		// first try to pass messages to one of topmost windows
		for( auto it = _topmost.rbegin(); _topmost.rend() != it; ++it )
		{
			// do not dispatch messages to disabled or invisible window
			if( (*it)->GetEnabled() && (*it)->GetVisible() )
			{
				// calculate absolute coordinates of the window
				float x_ = _desktop->GetX();
				float y_ = _desktop->GetY();
				for( Window *wnd = *it; _desktop.Get() != wnd; wnd = wnd->GetParent() )
				{
					assert(wnd);
					x_ += wnd->GetX();
					y_ += wnd->GetY();
				}
				if( ProcessMouseInternal(*it, x - x_, y - y_, z, msg) )
					return true;
			}
		}
		// then handle all children of the desktop recursively
		if( ProcessMouseInternal(_desktop.Get(), x, y, z, msg) )
			return true;
	}
	if( _hotTrackWnd.Get() )
	{
		_hotTrackWnd->OnMouseLeave();
		_hotTrackWnd.Set(nullptr);
	}
	return false;
}

bool LayoutManager::ProcessKeys(Msg msg, UI::Key key)
{
	switch( msg )
	{
	case MSGKEYUP:
		break;
	case MSGKEYDOWN:
		if( Window *wnd = GetFocusWnd() )
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
	if (Window *wnd = GetFocusWnd())
	{
		while (wnd)
		{
			if (wnd->OnChar(c))
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

void LayoutManager::Render(DrawingContext &dc) const
{
	dc.SetMode(RM_INTERFACE);

	// draw desktop and all its children
	if( _desktop->GetVisible() )
		_desktop->Draw(dc);

	// draw top-most windows
	for( const Window *w: _topmost )
	{
		if( w->GetVisible() )
		{
			float x = _desktop->GetX();
			float y = _desktop->GetY();
			for( Window *wnd = w->GetParent(); _desktop.Get() != wnd; wnd = wnd->GetParent() )
			{
				assert(wnd);
				x += wnd->GetX();
				y += wnd->GetY();
			}
			w->Draw(dc, x, y);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
