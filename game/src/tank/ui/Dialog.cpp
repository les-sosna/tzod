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
	Substrate(Window *parent) : Window(parent)
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
	SetBorder(true);

	_easyMove = false;

	GetManager()->SetFocusWnd(this);

//    if( _isModal = modal )
//	{
//		parent->Enable(false);
//	}
}

Dialog::~Dialog()
{
//	if( _isModal )
//	{
//		GetParent()->Enable(true);
//	}
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
		SetCapture();
		_mouseX = x;
		_mouseY = y;
	}
	return true;
}
bool Dialog::OnMouseUp(float x, float y, int button)
{
	if( 1 == button )
	{
		if( IsCaptured() )
		{
			ReleaseCapture();
		}
	}
	return true;
}
bool Dialog::OnMouseMove(float x, float y)
{
	if( IsCaptured() )
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

void Dialog::OnRawChar(int c)
{
	switch(c)
	{
	case VK_ESCAPE:
		Close(_resultCancel);
		break;
	}
}

bool Dialog::OnFocus(bool focus)
{
	return true;
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

