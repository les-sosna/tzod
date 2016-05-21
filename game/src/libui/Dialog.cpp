#include "inc/ui/Dialog.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/UIInput.h"

using namespace UI;

Dialog::Dialog(LayoutManager &manager, TextureManager &texman, float width, float height, bool modal)
  : Window(manager)
  , _mouseX(0)
  , _mouseY(0)
  , _easyMove(false)
{
	SetTexture(texman, "ui/window", false);
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
			eventClose(std::static_pointer_cast<Dialog>(shared_from_this()), result);
	}
}


//
// capture mouse messages
//

bool Dialog::OnPointerDown(InputContext &ic, float x, float y, int button, PointerType pointerType, unsigned int pointerID)
{
	if( _easyMove && 1 == button && !ic.HasCapturedPointers(this) )
	{
		ic.SetCapture(pointerID, shared_from_this());
		_mouseX = x;
		_mouseY = y;
	}
	return true;
}
bool Dialog::OnPointerUp(InputContext &ic, float x, float y, int button, PointerType pointerType, unsigned int pointerID)
{
	if( 1 == button && ic.GetCapture(pointerID).get() == this )
	{
		ic.SetCapture(pointerID, nullptr);
	}
	return true;
}
bool Dialog::OnPointerMove(InputContext &ic, float x, float y, PointerType pointerType, unsigned int pointerID)
{
	if( this == ic.GetCapture(pointerID).get())
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

void Dialog::NextFocus(bool wrap)
{
	auto &children = GetChildren();
	if (auto focus = GetFocus())
	{
		auto focusIt = std::find(children.rbegin(), children.rend(), focus);
		for (;;)
		{
			while (focusIt != children.rbegin())
			{
				focusIt--;
				if (TrySetFocus(*focusIt))
					return;
			}
			if (!wrap)
				break;
			wrap = false;
			focusIt = children.rend();
		}
	}
}

void Dialog::PrevFocus(bool wrap)
{
	auto &children = GetChildren();
	if (auto focus = GetFocus())
	{
		auto focusIt = std::find(children.begin(), children.end(), focus);
		for (;;)
		{
			while (focusIt != children.begin())
			{
				focusIt--;
				if (TrySetFocus(*focusIt))
					return;
			}
			if (!wrap)
				break;
			wrap = false;
			focusIt = children.end();
		}
	}
}

bool Dialog::TrySetFocus(const std::shared_ptr<Window> &child)
{
	if (child->GetVisible() && child->GetEnabled() && child->GetNeedsFocus())
	{
		SetFocus(child);
		return true;
	}
	return false;
}

bool Dialog::OnKeyPressed(InputContext &ic, Key key)
{
	bool shift = ic.GetInput().IsKeyPressed(Key::LeftShift) ||
		ic.GetInput().IsKeyPressed(Key::RightShift);

	switch( key )
	{
	case Key::Up:
		PrevFocus(false);
		break;
	case Key::Down:
		NextFocus(false);
		break;
	case Key::Tab:
		if (shift)
			PrevFocus(true);
		else
			NextFocus(true);
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

bool Dialog::GetNeedsFocus()
{
	return true;
}

