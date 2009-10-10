// GuiManager.cpp

#include "stdafx.h"
#include "GuiManager.h"

#include "MousePointer.h"

#include "video/RenderBase.h"
#include "video/TextureManager.h"


namespace UI
{

///////////////////////////////////////////////////////////////////////////////

LayoutManager::LayoutManager(IWindowFactory *pDesktopFactory) 
  : _captureCountSystem(0)
  , _captureCount(0)
  , _focusWnd(NULL)
  , _hotTrackWnd(NULL)
  , _captureWnd(NULL)
  , _desktop(NULL)
  , _isAppActive(false)
{
	_desktop.Set(pDesktopFactory->Create(this));
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
			_captureWnd.Set(NULL);
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
		for( PtrList<Window>::iterator it = _topmost.begin(); _topmost.end() != it; ++it )
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
	if( _focusWnd.Get() != wnd )
	{
		WindowWeakPtr wp(wnd);

		// reset old focus
		if( _focusWnd.Get() )
		{
			WindowWeakPtr tmp(_focusWnd.Get());
			_focusWnd.Set(NULL);
			tmp->OnFocus(false);
			if( tmp.Get() && tmp->eventLostFocus )
				INVOKE(tmp->eventLostFocus) ();
		}

		// try setting new focus
		if( wp.Get() && wnd->GetEnabled() && wnd->GetVisible()
			&& wnd->OnFocus(true) && wp.Get() )
		{
			assert(wnd->GetEnabled() && wnd->GetVisible());
			assert(!_focusWnd.Get());
			_focusWnd.Set(wnd);
		}
	}
	return NULL != _focusWnd.Get();
}

Window* LayoutManager::GetFocusWnd() const
{
	return _focusWnd.Get();
}

TextureManager* LayoutManager::GetTextureManager()
{
	return g_texman;
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
			SetFocusWnd(NULL);
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
		SetFocusWnd(NULL);

	if( _hotTrackWnd.Get() == wnd )
	{
		_hotTrackWnd->OnMouseLeave();
		_hotTrackWnd.Set(NULL);
	}

	if( _captureWnd.Get() == wnd )
	{
		_captureWnd.Set(NULL);
		_captureCount = 0;
	}
}

PtrList<Window>::iterator LayoutManager::TimeStepRegister(Window* wnd)
{
	_timestep.push_front(wnd);
	return _timestep.begin();
}

void LayoutManager::TimeStepUnregister(PtrList<Window>::iterator it)
{
	_timestep.safe_erase(it);
}

void LayoutManager::TimeStep(float dt)
{
	for( PtrList<Window>::safe_iterator it = _timestep.safe_begin(); it != _timestep.end(); ++it )
	{
		(*it)->OnTimeStep(dt);
	}
}

bool LayoutManager::ProcessMouseInternal(Window* wnd, float x, float y, float z, UINT msg)
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
			case WM_LBUTTONDOWN:  msgProcessed = wnd->OnMouseDown(x,y, 1);  break;
			case WM_RBUTTONDOWN:  msgProcessed = wnd->OnMouseDown(x,y, 2);  break;
			case WM_MBUTTONDOWN:  msgProcessed = wnd->OnMouseDown(x,y, 3);  break;

			case WM_LBUTTONUP:    msgProcessed = wnd->OnMouseUp(x,y, 1);    break;
			case WM_RBUTTONUP:    msgProcessed = wnd->OnMouseUp(x,y, 2);    break;
			case WM_MBUTTONUP:    msgProcessed = wnd->OnMouseUp(x,y, 3);    break;

			case WM_MOUSEMOVE:    msgProcessed = wnd->OnMouseMove(x,y);     break;

			case WM_MOUSEWHEEL:   msgProcessed = wnd->OnMouseWheel(x,y,z);  break;
		}
		// if window did not process the message, it should not destroy it self
		assert(!msgProcessed || wp.Get());

		if( wp.Get() && msgProcessed )
		{
			switch( msg )
			{
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
				SetFocusWnd(wnd); // may destroy wnd
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

bool LayoutManager::ProcessMouse(float x, float y, float z, UINT msg)
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
		for( PtrList<Window>::reverse_iterator it = _topmost.rbegin(); _topmost.rend() != it; ++it )
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
		_hotTrackWnd.Set(NULL);
	}
	return false;
}

bool LayoutManager::ProcessKeys(UINT msg, int c)
{
	switch( msg )
	{
	case WM_KEYUP:
		break;
	case WM_KEYDOWN:
		if( Window *wnd = _focusWnd.Get() )
		{
			while( wnd )
			{
				if( wnd->OnRawChar(c) )
				{
					return true;
				}
				wnd = wnd->GetParent();
			}
		}
		else
		{
			GetDesktop()->OnRawChar(c);
		}
		break;
	case WM_CHAR:
		if( Window *wnd = _focusWnd.Get() )
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
		break;
	default:
		assert(false);
	}

	return false;
}

void LayoutManager::Render() const
{
	g_render->SetMode(RM_INTERFACE);
	const DrawingContext *dc = static_cast<const DrawingContext*>(const_cast<LayoutManager*>(this)->GetTextureManager());

	// draw desktop and all its children
	if( _desktop->GetVisible() )
		_desktop->Draw(dc);

	// draw top-most windows
	for( PtrList<Window>::iterator it = _topmost.begin(); _topmost.end() != it; ++it )
	{
		if( (*it)->GetVisible() )
		{
			float x = _desktop->GetX();
			float y = _desktop->GetY();
			for( Window *wnd = (*it)->GetParent(); _desktop.Get() != wnd; wnd = wnd->GetParent() )
			{
				assert(wnd);
				x += wnd->GetX();
				y += wnd->GetY();
			}
			(*it)->Draw(dc, x, y);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
