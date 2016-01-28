#include "inc/ui/Dialog.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"

using namespace UI;

Dialog::Dialog(LayoutManager &manager, float width, float height, bool modal)
  : Window(manager)
  , _mouseX(0)
  , _mouseY(0)
  , _easyMove(false)
{
	SetTexture("ui/window", false);
	Resize(width, height);
	SetDrawBorder(true);
	SetDrawBackground(true);
}

void Dialog::SetEasyMove(bool enable)
{
	_easyMove = enable;
}

void Dialog::Close(int result)
{
	if (OnClose(result))
	{
		if (eventClose)
			eventClose(result);
		GetParent()->UnlinkChild(shared_from_this());
	}
}


//
// capture mouse messages
//

bool Dialog::OnPointerDown(float x, float y, int button, PointerType pointerType, unsigned int pointerID)
{
	if( _easyMove && 1 == button && !GetManager().HasCapturedPointers(this) )
	{
		GetManager().SetCapture(pointerID, shared_from_this());
		_mouseX = x;
		_mouseY = y;
	}
	return true;
}
bool Dialog::OnPointerUp(float x, float y, int button, PointerType pointerType, unsigned int pointerID)
{
	if( 1 == button && GetManager().GetCapture(pointerID).get() == this )
	{
		GetManager().SetCapture(pointerID, nullptr);
	}
	return true;
}
bool Dialog::OnPointerMove(float x, float y, PointerType pointerType, unsigned int pointerID)
{
	if( this == GetManager().GetCapture(pointerID).get())
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

bool Dialog::OnKeyPressed(Key key)
{
	switch( key )
	{
	case Key::Up:
		if( GetManager().GetFocusWnd() && this != GetManager().GetFocusWnd().get() )
		{
			// try to pass focus to previous siblings
			//auto r = GetManager().GetFocusWnd()->GetPrevSibling();
			//for( ; r; r = r->GetPrevSibling() )
			//{
			//	if( r->GetVisibleCombined() && r->GetEnabled() && GetManager().SetFocusWnd(r) ) break;
			//}
		}
		break;
	case Key::Down:
		if( GetManager().GetFocusWnd() && this != GetManager().GetFocusWnd().get() )
		{
			// try to pass focus to next siblings
			//auto r = GetManager().GetFocusWnd()->GetNextSibling();
			//for( ; r; r = r->GetNextSibling() )
			//{
			//	if( r->GetVisibleCombined() && r->GetEnabled() && GetManager().SetFocusWnd(r) ) break;
			//}
		}
		break;
	case Key::Tab:
		//if( GetManager().GetFocusWnd() && this != GetManager().GetFocusWnd().get() )
		//{
		//	// try to pass focus to next siblings ...
		//	auto r = GetManager().GetFocusWnd()->GetNextSibling();
		//	for( ; r; r = r->GetNextSibling() )
		//	{
		//		if( r->GetVisibleCombined() && r->GetEnabled() && GetManager().SetFocusWnd(r) ) break;
		//	}
		//	if( r ) break;

		//	// ... and then start from first one
		//	r = GetFirstChild();
		//	for( ; r; r = r->GetNextSibling() )
		//	{
		//		if( r->GetVisibleCombined() && r->GetEnabled() && GetManager().SetFocusWnd(r) ) break;
		//	}
		//}
		break;
	case Key::Enter:
	case Key::NumEnter:
		Close(_resultOK);
		break;
	case Key::Escape:
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

