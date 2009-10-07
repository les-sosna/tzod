// Dialog.cpp

#include "stdafx.h"

#include "Dialog.h"
#include "GuiManager.h"


namespace UI
{
/*
class Substrate : public Window
{
public:
	Substrate(Window *parent)
	  : Window(parent)
	{
		Resize(GetManager()->GetDesktop()->GetWidth(), GetManager()->GetDesktop()->GetHeight());
	}
};*/

///////////////////////////////////////////////////////////////////////////////
// Dialog class implementation

Dialog::Dialog(Window *parent, float width, float height, bool modal)
  : Window(/*modal ? new Substrate(parent) :*/ parent)
{
	Resize(width, height);
	Move((parent->GetWidth() - GetWidth()) * 0.5f, (parent->GetHeight() - GetHeight()) * 0.5f);
	SetDrawBorder(true);

	_easyMove = false;

	GetManager()->SetFocusWnd(this);
}

Dialog::~Dialog()
{
}

void Dialog::SetEasyMove(bool enable)
{
	_easyMove = enable;
}

void Dialog::Close(int result)
{
	if( eventClose )
		INVOKE(eventClose) (result);
	Destroy();
}


//
// capture mouse messages
//

bool Dialog::OnMouseDown(float x, float y, int button)
{
	if( _easyMove && 1 == button )
	{
		GetManager()->SetCapture(this);
		_mouseX = x;
		_mouseY = y;
	}
	return true;
}
bool Dialog::OnMouseUp(float x, float y, int button)
{
	if( 1 == button )
	{
		if( this == GetManager()->GetCapture() )
		{
			GetManager()->SetCapture(NULL);
		}
	}
	return true;
}
bool Dialog::OnMouseMove(float x, float y)
{
	if( this == GetManager()->GetCapture() )
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
	case VK_UP:
		if( GetManager()->GetFocusWnd() && this != GetManager()->GetFocusWnd() )
		{
			// try to pass focus to previous siblings
			UI::Window *r = GetManager()->GetFocusWnd()->GetPrevSibling();
			for( ; r; r = r->GetPrevSibling() )
			{
				if( !r->GetVisible() || !r->GetEnabled() || r->IsDestroyed() ) continue;
				if( GetManager()->SetFocusWnd(r) ) break;
			}
		}
		break;
	case VK_DOWN:
		if( GetManager()->GetFocusWnd() && this != GetManager()->GetFocusWnd() )
		{
			// try to pass focus to next siblings
			UI::Window *r = GetManager()->GetFocusWnd()->GetNextSibling();
			for( ; r; r = r->GetNextSibling() )
			{
				if( !r->GetVisible() || !r->GetEnabled() || r->IsDestroyed() ) continue;
				if( GetManager()->SetFocusWnd(r) ) break;
			}
		}
		break;
	case VK_TAB:
		if( GetManager()->GetFocusWnd() && this != GetManager()->GetFocusWnd() )
		{
			// try to pass focus to next siblings ...
			UI::Window *r = GetManager()->GetFocusWnd()->GetNextSibling();
			for( ; r; r = r->GetNextSibling() )
			{
				if( !r->GetVisible() || !r->GetEnabled() || r->IsDestroyed() ) continue;
				if( GetManager()->SetFocusWnd(r) ) break;
			}
			if( r ) break;

			// ... and then start from first one
			r = GetFirstChild();
			for( ; r; r = r->GetNextSibling() )
			{
				if( !r->GetVisible() || !r->GetEnabled() || r->IsDestroyed() ) continue;
				if( GetManager()->SetFocusWnd(r) ) break;
			}
		}
		break;
	case VK_ESCAPE:
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

