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
  : _focusWnd(NULL)
  , _hotTrackWnd(NULL)
  , _captureWnd(NULL)
  , _captureCountSystem(0)
  , _captureCount(0)
  , _windowCount(0)
  , _isAppActive(false)
{
	_desktop = pDesktopFactory->Create(this);
}

LayoutManager::~LayoutManager()
{
	_desktop->Destroy();
	_desktop = NULL;
	assert(_topmost.empty());
	assert(0 == GetWndCount());
}

void LayoutManager::Add(Window* wnd)
{
	assert(wnd);
	++_windowCount;
}

void LayoutManager::Remove(Window* wnd)
{
	assert(!wnd->GetTopMost()); // can't remove top most window

	if( wnd == _hotTrackWnd )
	{
		_hotTrackWnd = NULL;
	}
	if( wnd == _focusWnd )
	{
		ResetFocus(wnd);
		assert(wnd != _focusWnd);
	}

	if( wnd == _captureWnd )
	{
		_captureWnd   = NULL;
		_captureCount = 0;
	}

	--_windowCount;
}

Window* LayoutManager::GetCapture() const
{
	return _captureWnd;
}

unsigned int LayoutManager::GetWndCount() const
{
	return _windowCount;
}

Window* LayoutManager::GetDesktop() const
{
	return _desktop;
}

void LayoutManager::SetCapture(Window* wnd)
{
	if( wnd )
	{
		if( _captureWnd )
		{
			assert(_captureWnd == wnd);
			assert(_captureCount != 0);
		}
		else
		{
			assert(0 == _captureCount);
			_captureWnd = wnd;
		}
		_captureCount++;
	}
	else
	{
		assert(_captureWnd);
		assert(0 != _captureCount);
		if( 0 == --_captureCount )
		{
			_captureWnd = NULL;
		}
	}
}

void LayoutManager::AddTopMost(Window* wnd, bool add)
{
	if( add )
	{
		assert(!wnd->IsDestroyed());
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
	if( wnd )
	{
		assert(wnd->GetEnabled() && wnd->GetVisible());

		if( _focusWnd == wnd )
			return true;

		if( wnd->OnFocus(true) )
		{
			if( _focusWnd )
			{
				_focusWnd->OnFocus(false);
				if( _focusWnd->eventLostFocus )
					INVOKE(_focusWnd->eventLostFocus) ();
			}
			_focusWnd = wnd;
			return true;
		}
	}
	else if( _focusWnd )
	{
		_focusWnd->OnFocus(false);
		if( _focusWnd->eventLostFocus )
			INVOKE(_focusWnd->eventLostFocus) ();
		_focusWnd = NULL;
	}

	return false;
}

Window* LayoutManager::GetFocusWnd() const
{
	return _focusWnd;
}

TextureManager* LayoutManager::GetTextureManager() const
{
	return g_texman;
}

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
			if( !w->GetVisible() || !w->GetEnabled() || w->IsDestroyed() )
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
				if( !r->GetVisible() || !r->GetEnabled() || r->IsDestroyed() ) continue;
				if( SetFocusWnd(r) ) break;
			}
			if( r ) break;

			// try to pass focus to previous siblings
			for( r = tmp->GetPrevSibling(); r; r = r->GetPrevSibling() )
			{
				if( !r->GetVisible() || !r->GetEnabled() || r->IsDestroyed() ) continue;
				if( SetFocusWnd(r) ) break;
			}
			if( r ) break;

			// and finally try to pass focus to the parent and its siblings
			tmp = tmp->GetParent();
			assert(!tmp || (tmp->GetVisible() && tmp->GetEnabled() && !tmp->IsDestroyed()));
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

Window* LayoutManager::GetHotTrackWnd() const
{
	return _hotTrackWnd;
}

void LayoutManager::ResetHotTrackWnd(Window* wnd)
{
	if( _hotTrackWnd == wnd )
	{
		_hotTrackWnd->OnMouseLeave();
		_hotTrackWnd = NULL;
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
		//
		// route message to each child until someone process it
		//

		if( !wnd->GetClipChildren() || bMouseInside )
		{
			for( Window *w = wnd->GetLastChild(); w; w = w->GetPrevSibling() )
			{
				// do not dispatch messages to disabled or invisible
				// topmost windows are processed in different way
				if( !w->GetEnabled() || !w->GetVisible() || w->GetTopMost() )
					continue;
				if( ProcessMouseInternal(w, x - w->GetX(), y - w->GetY(), z, msg) )
					return true;
			}
		}
	}

	if( bMouseInside || GetCapture() == wnd )
	{
		//
		// window is captured or mouse pointer is inside the window
		//

		wnd->AddRef(); // to be sure that pointer is valid if window was destroyed

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

		if( !wnd->IsDestroyed() && wnd->GetEnabled() && wnd->GetVisible() && msgProcessed )
		{
			switch( msg )
			{
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
				if( _focusWnd != wnd )
				{
					SetFocusWnd(wnd);
				}
			}

			if( wnd != _hotTrackWnd )
			{
				if( _hotTrackWnd )
					_hotTrackWnd->OnMouseLeave();
				if( wnd->GetVisible() && wnd->GetEnabled() )
				{
					_hotTrackWnd = wnd;
					_hotTrackWnd->OnMouseEnter(x, y);
				}
			}
		}

		wnd->Release();
		return msgProcessed;
	}

	return false;
}

bool LayoutManager::ProcessMouse(float x, float y, float z, UINT msg)
{
	bool msgProcessed = false;

	if( _captureWnd )
	{
		// we have a captured window
		// calc relative mouse position and route message to captured window
		for( Window *wnd = _captureWnd; _desktop != wnd; wnd = wnd->GetParent() )
		{
			assert(wnd);
			x -= wnd->GetX();
			y -= wnd->GetY();
		}
		msgProcessed = ProcessMouseInternal(_captureWnd, x, y, z, msg);
	}
	else
	{
		//
		// first try to pass messages to one of the topmost windows
		//
		for( PtrList<Window>::reverse_iterator it = _topmost.rbegin(); _topmost.rend() != it; ++it )
		{
			if( !(*it)->GetEnabled() || !(*it)->GetVisible() )
				continue;  // do not dispatch messages to disabled or invisible window

			// calculate absolute coordinates of the window
			float x_ = _desktop->GetX();
			float y_ = _desktop->GetY();
			for( Window *wnd = *it; _desktop != wnd; wnd = wnd->GetParent() )
			{
				assert(wnd);
				x_ += wnd->GetX();
				y_ += wnd->GetY();
			}
			msgProcessed = ProcessMouseInternal(*it, x - x_, y - y_, z, msg);
			if( msgProcessed ) break;
		}

		//
		// then handle all children of the desktop recursively
		//
		if( !msgProcessed )
		{
			msgProcessed = ProcessMouseInternal(_desktop, x, y, z, msg);
		}
	}

	if( !msgProcessed && _hotTrackWnd )
	{
		_hotTrackWnd->OnMouseLeave();
		_hotTrackWnd = NULL;
	}

	return msgProcessed;
}

bool LayoutManager::ProcessKeys(UINT msg, int c)
{
	switch( msg )
	{
	case WM_KEYUP:
		break;
	case WM_KEYDOWN:
		if( Window *wnd = _focusWnd )
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
		if( Window *wnd = _focusWnd )
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
	const DrawingContext *dc = static_cast<const DrawingContext*>(GetTextureManager());

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
			for( Window *wnd = (*it)->GetParent(); _desktop != wnd; wnd = wnd->GetParent() )
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
