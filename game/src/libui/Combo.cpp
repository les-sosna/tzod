#include "inc/ui/Combo.h"
#include "inc/ui/Text.h"
#include "inc/ui/List.h"
#include "inc/ui/Button.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"

using namespace UI;

ComboBox::ComboBox(LayoutManager &manager, ListDataSource *dataSource)
  : Window(manager)
  , _curSel(-1)
{
	_list = std::make_shared<List>(manager, dataSource);
	_list->SetTexture("ui/combo_list", false);
	_list->SetVisible(false);
	_list->SetTopMost(true);
	_list->eventClickItem = std::bind(&ComboBox::OnClickItem, this, std::placeholders::_1);
	_list->eventChangeCurSel = std::bind(&ComboBox::OnChangeSelection, this, std::placeholders::_1);
	_list->eventLostFocus = std::bind(&ComboBox::OnListLostFocus, this);
	AddFront(_list);

	_btn = std::make_shared<ImageButton>(manager);
	_btn->SetTexture("ui/scroll_down", true);
	_btn->eventClick = std::bind(&ComboBox::DropList, this);
	AddFront(_btn);

	_text = std::make_shared<TextButton>(manager);
	_text->Move(0, 1);
	_text->SetFont("font_small");
	_text->eventClick = std::bind(&ComboBox::DropList, this);
	_text->SetDrawShadow(false);
	AddFront(_text);

	SetDrawBorder(true);
	SetTexture("ui/combo", false);
	Window::Resize(GetWidth(), _text->GetHeight() + _text->GetY() * 2);
}

ListDataSource* ComboBox::GetData() const
{
	return _list->GetData();
}

void ComboBox::SetCurSel(int index)
{
	_curSel = index;
	_list->SetCurSel(index);
	if( eventChangeCurSel )
		eventChangeCurSel(index);
}

int ComboBox::GetCurSel() const
{
	return _curSel;
}

std::shared_ptr<List> ComboBox::GetList() const
{
	return _list;
}

void ComboBox::DropList()
{
	if( _list->GetVisible() )
	{
		_btn->SetTexture("ui/scroll_down", false);
		_list->SetVisible(false);
		_list->SetCurSel(GetCurSel());
		GetManager().SetFocusWnd(shared_from_this());
	}
	else
	{
		_btn->SetTexture("ui/scroll_up", false);
		_list->SetVisible(true);
		_list->SetScrollPos((float) GetCurSel());
		GetManager().SetFocusWnd(_list);
	}
}

void ComboBox::OnClickItem(int index)
{
	if( -1 != index )
	{
		_curSel = index;
		_text->SetText(_list->GetData()->GetItemText(index, 0));
		_text->Resize(_btn->GetX(), GetHeight()); // workaround: SetText changes button size
		_list->SetVisible(false);
		_btn->SetTexture("ui/scroll_down", false);
		GetManager().SetFocusWnd(shared_from_this());

		if( eventChangeCurSel )
			eventChangeCurSel(index);
	}
}

void ComboBox::OnChangeSelection(int index)
{
	if( -1 != index )
	{
		_text->SetText(_list->GetData()->GetItemText(index, 0));
		_text->Resize(_btn->GetX(), GetHeight()); // workaround: SetText changes button size
	}
}

void ComboBox::OnListLostFocus()
{
	_list->SetCurSel(_curSel); // cancel changes
	_list->SetVisible(false);
	_btn->SetTexture("ui/scroll_down", false);
}

void ComboBox::OnEnabledChange(bool enable, bool inherited)
{
	SetFrame(enable ? 0 : 3);
}

bool ComboBox::OnKeyPressed(Key key)
{
	switch( key )
	{
	case Key::Escape:
		if( _list->GetVisible() )
		{
			GetManager().SetFocusWnd(shared_from_this());
			return true;
		}
		break;
	case Key::Enter:
		if( _list->GetVisible() )
		{
			if( -1 != _list->GetCurSel() )
			{
				OnClickItem(_list->GetCurSel());
				return true;
			}
		}
		break;
	case Key::Down:
		if( !_list->GetVisible() )
			DropList();
		return true;
    default:
        break;
	}
	return false;
}

bool ComboBox::OnFocus(bool focus)
{
	return true;
}

void ComboBox::OnSize(float width, float height)
{
	_list->Move(0, height);
	_list->Resize(width, _list->GetHeight());
	_btn->Move(width - _btn->GetWidth(), (height - _btn->GetHeight()) * 0.5f);
	_text->Resize(_btn->GetX(), height);
}
