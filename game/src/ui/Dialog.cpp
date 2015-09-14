// Dialog.cpp

#include "inc/ui/Dialog.h"
#include "inc/ui/GuiManager.h"
#include <GLFW/glfw3.h>

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// Dialog class implementation

Dialog::Dialog(Window *parent, float width, float height, bool modal)
  : Window(/*modal ? new Substrate(parent) :*/ parent)
  , _mouseX(0)
  , _mouseY(0)
  , _easyMove(false)
{
	SetTexture("ui/window", false);
	Resize(width, height);
    Move(std::floor((parent->GetWidth() - GetWidth()) / 2), std::floor((parent->GetHeight() - GetHeight()) / 2));
	SetDrawBorder(true);
	SetDrawBackground(true);
	GetManager().SetFocusWnd(this);
}

void Dialog::SetEasyMove(bool enable)
{
	_easyMove = enable;
}

void Dialog::Close(int result)
{
	if( eventClose )
		eventClose(result);
	Destroy();
}


//
// capture mouse messages
//

bool Dialog::OnMouseDown(float x, float y, int button)
{
	if( _easyMove && 1 == button )
	{
		GetManager().SetCapture(this);
		_mouseX = x;
		_mouseY = y;
	}
	return true;
}
bool Dialog::OnMouseUp(float x, float y, int button)
{
	if( 1 == button )
	{
		if( this == GetManager().GetCapture() )
		{
			GetManager().SetCapture(nullptr);
		}
	}
	return true;
}
bool Dialog::OnMouseMove(float x, float y)
{
	if( this == GetManager().GetCapture() )
	{
		Move(GetX() + x - _mouseX, GetY() + y - _mouseY);
	}
	return true;
}
bool Dialog::OnMouseEnter(float x, float y)
{
	return true;
}
bool Dialog::OnMouseLeave()
{
	return true;
}

bool Dialog::OnRawChar(int c)
{
	switch( c )
	{
	case GLFW_KEY_UP:
		if( GetManager().GetFocusWnd() && this != GetManager().GetFocusWnd() )
		{
			// try to pass focus to previous siblings
			Window *r = GetManager().GetFocusWnd()->GetPrevSibling();
			for( ; r; r = r->GetPrevSibling() )
			{
				if( r->GetVisible() && r->GetEnabled() && GetManager().SetFocusWnd(r) ) break;
			}
		}
		break;
	case GLFW_KEY_DOWN:
		if( GetManager().GetFocusWnd() && this != GetManager().GetFocusWnd() )
		{
			// try to pass focus to next siblings
			Window *r = GetManager().GetFocusWnd()->GetNextSibling();
			for( ; r; r = r->GetNextSibling() )
			{
				if( r->GetVisible() && r->GetEnabled() && GetManager().SetFocusWnd(r) ) break;
			}
		}
		break;
	case GLFW_KEY_TAB:
		if( GetManager().GetFocusWnd() && this != GetManager().GetFocusWnd() )
		{
			// try to pass focus to next siblings ...
			Window *r = GetManager().GetFocusWnd()->GetNextSibling();
			for( ; r; r = r->GetNextSibling() )
			{
				if( r->GetVisible() && r->GetEnabled() && GetManager().SetFocusWnd(r) ) break;
			}
			if( r ) break;

			// ... and then start from first one
			r = GetFirstChild();
			for( ; r; r = r->GetNextSibling() )
			{
				if( r->GetVisible() && r->GetEnabled() && GetManager().SetFocusWnd(r) ) break;
			}
		}
		break;
	case GLFW_KEY_ESCAPE:
		Close(_resultCancel);
		break;
	default:
		return false;
	}
	return true;
}

bool Dialog::OnFocus(bool focus)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

