#include "inc/ui/Combo.h"
#include "inc/ui/Text.h"
#include "inc/ui/List.h"
#include "inc/ui/Button.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"

using namespace UI;

ComboBox::ComboBox(LayoutManager &manager, TextureManager &texman, ListDataSource *dataSource)
  : Window(manager)
  , _curSel(-1)
{
	_list = std::make_shared<List>(manager, texman, dataSource);
	_list->SetTexture(texman, "ui/combo_list", false);
	_list->SetVisible(false);
	_list->SetTopMost(true);
	_list->eventClickItem = std::bind(&ComboBox::OnClickItem, this, std::placeholders::_1);
	_list->eventChangeCurSel = std::bind(&ComboBox::OnChangeSelection, this, std::placeholders::_1);
	_list->eventLostFocus = std::bind(&ComboBox::OnListLostFocus, this);
	AddFront(_list);

	_btn = std::make_shared<ImageButton>(manager);
	_btn->SetTexture(texman, "ui/scroll_down", true);
	_btn->eventClick = std::bind(&ComboBox::DropList, this);
	AddFront(_btn);

	_text = std::make_shared<TextButton>(manager, texman);
	_text->Move(0, 1);
	_text->SetFont(texman, "font_small");
	_text->eventClick = std::bind(&ComboBox::DropList, this);
	_text->SetDrawShadow(false);
	AddFront(_text);

	SetDrawBorder(true);
	SetTexture(texman, "ui/combo", false);
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
		_btn->SetTexture(GetManager().GetTextureManager(), "ui/scroll_down", false);
		_list->SetVisible(false);
		_list->SetCurSel(GetCurSel());
		SetFocus(nullptr);
	}
	else
	{
		_btn->SetTexture(GetManager().GetTextureManager(), "ui/scroll_up", false);
		_list->SetVisible(true);
		_list->SetScrollPos((float) GetCurSel());
		SetFocus(_list);
	}
}

void ComboBox::OnClickItem(int index)
{
	if( -1 != index )
	{
		_curSel = index;
		_text->SetText(GetManager().GetTextureManager(), _list->GetData()->GetItemText(index, 0));
		_text->Resize(_btn->GetX(), GetHeight()); // workaround: SetText changes button size
		_list->SetVisible(false);
		_btn->SetTexture(GetManager().GetTextureManager(), "ui/scroll_down", false);
		SetFocus(nullptr);

		if( eventChangeCurSel )
			eventChangeCurSel(index);
	}
}

void ComboBox::OnChangeSelection(int index)
{
	if( -1 != index )
	{
		_text->SetText(GetManager().GetTextureManager(), _list->GetData()->GetItemText(index, 0));
		_text->Resize(_btn->GetX(), GetHeight()); // workaround: SetText changes button size
	}
}

void ComboBox::OnListLostFocus()
{
	_list->SetCurSel(_curSel); // cancel changes
	_list->SetVisible(false);
	_btn->SetTexture(GetManager().GetTextureManager(), "ui/scroll_down", false);
}

void ComboBox::OnEnabledChange(bool enable, bool inherited)
{
	SetFrame(enable ? 0 : 3);
}

bool ComboBox::OnKeyPressed(InputContext &ic, Key key)
{
	switch( key )
	{
	case Key::Escape:
		if( _list->GetVisible() )
		{
			SetFocus(nullptr);
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

void ComboBox::OnSize(float width, float height)
{
	_list->Move(0, height);
	_list->Resize(width, _list->GetHeight());
	_btn->Move(width - _btn->GetWidth(), (height - _btn->GetHeight()) * 0.5f);
	_text->Resize(_btn->GetX(), height);
}
