#include "GetFileName.h"

#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <ui/Text.h>
#include <ui/List.h>
#include <ui/ListBox.h>
#include <ui/Button.h>
#include <ui/Edit.h>
#include <ui/DataSourceAdapters.h>
#include <ui/GuiManager.h>
#include <ui/Keys.h>

GetFileNameDlg::GetFileNameDlg(UI::LayoutManager &manager, TextureManager &texman, const Params &param, LangCache &lang)
  : Dialog(manager, texman, 512, 460)
  , _folder(param.folder)
  , _changing(false)
{
	auto t = std::make_shared<UI::Text>(manager, texman);
	t->Move(GetWidth() / 2, 16);
	t->SetText(texman, param.title);
	t->SetAlign(alignTextCT);
	t->SetFont(texman, "font_default");
	AddFront(t);

	_ext = param.extension;
	_files = std::make_shared<DefaultListBox>(manager, texman);
	_files->Move(20, 56);
	_files->Resize(472, 300);
	AddFront(_files);
	if( _folder )
	{
		auto files = _folder->EnumAllFiles("*." + _ext);
		for( auto it = files.begin(); it != files.end(); ++it )
		{
			it->erase(it->length() - _ext.length() - 1); // cut out the file extension
			_files->GetData()->AddItem(*it);
		}
	}
	_files->GetData()->Sort();
	_files->GetList()->eventChangeCurSel = std::bind(&GetFileNameDlg::OnSelect, this, std::placeholders::_1);

	auto text = std::make_shared<UI::Text>(manager, texman);
	text->Move(16, 370);
	text->SetText(texman, lang.get_file_name_title.Get());
	AddFront(text);

	_fileName = std::make_shared<UI::Edit>(manager, texman);
	_fileName->Move(20, 385);
	_fileName->Resize(472, _fileName->GetHeight());
	_fileName->eventChange = std::bind(&GetFileNameDlg::OnChangeName, this);
	AddFront(_fileName);

	auto btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(texman, lang.common_ok.Get());
	btn->Move(290, 420);
	btn->eventClick = std::bind(&GetFileNameDlg::OnOK, this);
	AddFront(btn);

	btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(texman, lang.common_cancel.Get());
	btn->Move(400, 420);
	btn->eventClick = std::bind(&GetFileNameDlg::OnCancel, this);
	AddFront(btn);

	SetFocus(_fileName);
}

GetFileNameDlg::~GetFileNameDlg()
{
}

std::string GetFileNameDlg::GetFileName() const
{
	return _fileName->GetText() + "." + _ext;
}

std::string GetFileNameDlg::GetFileTitle() const
{
	return _fileName->GetText();
}

void GetFileNameDlg::OnSelect(int index)
{
	if( _changing || -1 == index ) return;
	_fileName->SetText(GetManager().GetTextureManager(), _files->GetData()->GetItemText(index, 0));
}

void GetFileNameDlg::OnChangeName()
{
	_changing = true;
	size_t match = 0;
	std::string txt = _fileName->GetText();
	for( int i = 0; i < _files->GetData()->GetItemCount(); ++i )
	{
		std::string fn = _files->GetData()->GetItemText(i, 0);
		size_t n = 0;
		while( n < fn.length() && n < txt.length() )
		{
			if( fn[n] != txt[n] ) break;
			++n;
		}
		if( n > match )
		{
			match = n;
			_files->GetList()->SetCurSel(i, true);
		}
	}
	_changing = false;
}

bool GetFileNameDlg::OnKeyPressed(UI::InputContext &ic, UI::Key key)
{
	switch( key )
	{
	//case UI::Key::Up:
	//case UI::Key::Down:
	//	static_cast<Window *>(_files)->OnKeyPressed(c);
	//	break;
	case UI::Key::Enter:
		OnOK();
		break;
	default:
		return Dialog::OnKeyPressed(ic, key);
	}
	return true;
}

void GetFileNameDlg::OnOK()
{
	Close(_resultOK);
}

void GetFileNameDlg::OnCancel()
{
	Close(_resultCancel);
}
