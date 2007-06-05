// GuiManager.cpp

#include "stdafx.h"
#include "GuiManager.h"

#include "MousePointer.h"

#include "video/RenderBase.h"

///////////////////////////////////////////////////////////////////////////////

GuiManager::GuiManager(CreateWindowProc createDesctop)
{
	_focusWnd      = NULL;
	_hotTrackWnd   = NULL;
	_captureWnd    = NULL;
	_captureCount  = 0;
	_windowCount   = 0;

	_desktop = createDesctop(this);
	_cursor  = new UI::MouseCursor(this, "cursor");
}

GuiManager::~GuiManager()
{
	_desktop->Destroy();
	_desktop = NULL;

	_cursor->Destroy();
	_cursor = NULL;

	_ASSERT(0 == GetWndCount());
}

void GuiManager::Add(UI::Window* wnd)
{
	_ASSERT(wnd);
	++_windowCount;
}

void GuiManager::Remove(UI::Window* wnd)
{
	_ASSERT(!wnd->IsTopMost()); // can't remove top most window

	if( wnd == _hotTrackWnd )
	{
		_hotTrackWnd = NULL;
	}
	if( wnd == _focusWnd )
	{
		UI::Window *tmp = wnd;
		while(1)
		{
			UI::Window *r;

			// try to pass focus to next siblings
			for( r = tmp->GetNextSibling(); r; r = r->GetNextSibling() )
			{
				if( !r->IsVisible() || !r->IsEnabled() || r->IsDestroyed() ) continue;
				if( SetFocusWnd(r) ) break;
			}
			if( r ) break;

			// try to pass focus to previous siblings
			for( r = tmp->GetPrevSibling(); r; r = r->GetPrevSibling() )
			{
				if( !r->IsVisible() || !r->IsEnabled() || r->IsDestroyed() ) continue;
				if( SetFocusWnd(r) ) break;
			}
			if( r ) break;

			// and finaly try to pass focus to the parent and its siblings
			tmp = tmp->GetParent();
			if( !tmp || (tmp->IsVisible() && tmp->IsEnabled()
				&& !tmp->IsDestroyed() && SetFocusWnd(tmp)) )
			{
				break;
			}
		}// while(1)
	}

	if( wnd == _captureWnd )
	{
		_captureWnd   = NULL;
		_captureCount = 0;
	}

	--_windowCount;
}

UI::Window* GuiManager::GetCapture() const
{
	return _captureWnd;
}

int GuiManager::GetWndCount() const
{
	return _windowCount;
}

UI::Window* GuiManager::GetDesktop() const
{
	return _desktop;
}

void GuiManager::SetCapture(UI::Window* wnd)
{
	if( _captureWnd )
	{
		_ASSERT(_captureWnd == wnd);
		_ASSERT(_captureCount != 0);
	}
	else
	{
		_ASSERT(0 == _captureCount);
		_captureWnd = wnd;
	}
	_captureCount++;
}

void GuiManager::ReleaseCapture(UI::Window* wnd)
{
	_ASSERT(wnd == _captureWnd);
	_ASSERT(0 != _captureCount);

	if( 0 == --_captureCount )
	{
		_captureWnd = NULL;
	}
}

void GuiManager::AddTopMost(UI::Window* wnd, bool add)
{
	if( add )
	{
		_ASSERT(!wnd->IsDestroyed());
		_topmost.push_back(wnd);
	}
	else
	{
		_topmost.remove(wnd);
	}
}

void GuiManager::PushClippingRect(const RECT &rect)
{
	if( _clipStack.empty() )
	{
		_clipStack.push(rect);
		g_render->SetViewport(&rect);
	}
	else
	{
		RECT tmp = _clipStack.top();
		if( rect.left   > tmp.left )   tmp.left = rect.left;
		if( rect.top    > tmp.top )    tmp.top = rect.top;
		if( rect.right  < tmp.right )  tmp.right = rect.right;
		if( rect.bottom < tmp.bottom ) tmp.bottom = rect.bottom;
		_clipStack.push(tmp);
		g_render->SetViewport(&tmp);
	}
}

void GuiManager::PopClippingRect()
{
	_ASSERT(!_clipStack.empty());
	_clipStack.pop();
	if( _clipStack.empty() )
		g_render->SetViewport(NULL);
	else
		g_render->SetViewport(&_clipStack.top());
}

bool GuiManager::SetFocusWnd(UI::Window* wnd)
{
	if( wnd )
	{
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

UI::Window* GuiManager::GetFocusWnd() const
{
	return _focusWnd;
}

PtrList<UI::Window>::iterator GuiManager::TimeStepRegister(UI::Window* wnd)
{
	_timestep.push_front(wnd);
	return _timestep.begin();
}

void GuiManager::TimeStepUnregister(PtrList<UI::Window>::iterator it)
{
	_timestep.safe_erase(it);
}

void GuiManager::TimeStep(float dt)
{
	PtrList<UI::Window>::safe_iterator it = _timestep.safe_begin();
	while( it != _timestep.end() )
	{
		(*it)->OnTimeStep(dt);
		++it;
	}
}

bool GuiManager::_ProcessMouse(UI::Window* wnd, float x, float y, float z, UINT msg)
{
	if( !wnd->IsCaptured() )
	{
		//
		// route message to each child until someone process it
		//
		for( UI::Window *w = wnd->GetLastChild(); w; w = w->GetPrevSibling() )
		{
			if( !w->IsEnabled() || !w->IsVisible() )
				continue; // do not dispatch messages to disabled or invisible window

			if( _ProcessMouse(w, x - w->GetX(), y - w->GetY(), z, msg) )
				return true;
		}
	}

	if( (x >= 0 && x < wnd->GetWidth() && y >= 0 && y < wnd->GetHeight())
		|| wnd->IsCaptured() )
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

		if( !wnd->IsDestroyed() && msgProcessed )
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
				_hotTrackWnd = wnd;
				_hotTrackWnd->OnMouseEnter(x, y);
			}
		}

		wnd->Release();
		return msgProcessed;
	}

	return false;
}

bool GuiManager::ProcessMouse(float x, float y, float z, UINT msg)
{
	bool msgProcessed;

	if( _captureWnd )
	{
		// we have a captured window
		// calc relative mouse position and route message to captured window
		for( UI::Window *wnd = _captureWnd; _desktop != wnd; wnd = wnd->GetParent() )
		{
			_ASSERT(wnd);
			x -= wnd->GetX();
			y -= wnd->GetY();
		}
		msgProcessed = _ProcessMouse(_captureWnd, x, y, z, msg);
	}
	else
	{
		msgProcessed = _ProcessMouse(_desktop, x, y, z, msg);
	}

	if( !msgProcessed && _hotTrackWnd )
	{
		_hotTrackWnd->OnMouseLeave();
		_hotTrackWnd = NULL;
	}

	return msgProcessed;
}

bool GuiManager::ProcessKeys(UINT msg, int c)
{
    switch( msg )
	{
	case WM_KEYUP:
		break;
	case WM_KEYDOWN:
		if( _focusWnd )
			_focusWnd->OnRawChar(c);
		else
			GetDesktop()->OnRawChar(c);
		break;
	case WM_CHAR:
		if( _focusWnd )
			_focusWnd->OnChar(c);
		else
			GetDesktop()->OnChar(c);
		break;
	default:
		_ASSERT(FALSE);
	}

	return true;
}

void GuiManager::Resize(float width, float height)
{
	_desktop->Resize(width, height);
}

void GuiManager::Show(bool show)
{
	_desktop->Show(show);
}

void GuiManager::Draw()
{
	_ASSERT(_clipStack.empty());

	_desktop->Draw();

	std::list<UI::Window*>::iterator it = _topmost.begin();
	for( ; _topmost.end() != it; ++it )
	{
		float x = _desktop->GetX();
		float y = _desktop->GetY();
		for( UI::Window *wnd = (*it)->GetParent(); _desktop != wnd; wnd = wnd->GetParent() )
		{
			_ASSERT(wnd);
			x += wnd->GetX();
			y += wnd->GetY();
		}
		(*it)->Draw(x, y);
	}

	_cursor->Draw();

	_ASSERT(_clipStack.empty());
}

///////////////////////////////////////////////////////////////////////////////
// end of file
