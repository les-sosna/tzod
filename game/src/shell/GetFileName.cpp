#include "GetFileName.h"
#include <cbind/ConfigBinding.h>
#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <plat/Keys.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/DataSourceAdapters.h>
#include <ui/Edit.h>
#include <ui/EditableText.h>
#include <ui/List.h>
#include <ui/ListBox.h>
#include <ui/Text.h>

#include <algorithm>

GetFileNameDlg::GetFileNameDlg(const Params &param, LangCache &lang)
  : _folder(param.folder)
  , _changing(false)
{
	Resize(512, 460);

	auto t = std::make_shared<UI::Text>();
	t->Move(GetWidth() / 2, 16);
	t->SetText(std::make_shared<UI::StaticText>(param.title));
	t->SetAlign(alignTextCT);
	t->SetFont("font_default");
	AddFront(t);

	_ext = param.extension;
	_files = std::make_shared<DefaultListBox>();
	_files->Move(20, 56);
	_files->Resize(472, 300);
	AddFront(_files);

	if (!param.blank.empty())
	{
		_files->GetData()->AddItem(param.blank);
	}

	if( _folder )
	{
		auto files = _folder->EnumAllFiles("*." + _ext);
		std::sort(files.begin(), files.end());
		for( auto it = files.begin(); it != files.end(); ++it )
		{
			it->erase(it->length() - _ext.length() - 1); // cut out the file extension
			_files->GetData()->AddItem(*it);
		}
	}
	_files->GetList()->SetCurSel(0);
	_files->GetList()->eventChangeCurSel = std::bind(&GetFileNameDlg::OnSelect, this, std::placeholders::_1);

	auto text = std::make_shared<UI::Text>();
	text->Move(16, 370);
	text->SetText(ConfBind(lang.get_file_name_title));
	AddFront(text);

	_fileName = std::make_shared<UI::Edit>();
	_fileName->Move(20, 385);
	_fileName->SetWidth(472);
	_fileName->GetEditable()->eventChange = std::bind(&GetFileNameDlg::OnChangeName, this);
	AddFront(_fileName);

	auto btn = std::make_shared<UI::Button>();
	btn->SetText(ConfBind(lang.common_ok));
	btn->Move(400, 420);
	btn->eventClick = std::bind(&GetFileNameDlg::OnOK, this);
	AddFront(btn);

	SetFocus(_fileName);
}

GetFileNameDlg::~GetFileNameDlg()
{
}

bool GetFileNameDlg::IsBlank() const
{
	return _files->GetList()->GetCurSel() == 0;
}

std::string GetFileNameDlg::GetFileName() const
{
	return std::string(_fileName->GetEditable()->GetText()).append(".").append(_ext);
}

std::string_view GetFileNameDlg::GetFileTitle() const
{
	return _fileName->GetEditable()->GetText();
}

void GetFileNameDlg::OnSelect(int index)
{
	if( _changing || -1 == index ) return;
	_fileName->GetEditable()->SetText(std::string(_files->GetData()->GetItemText(index, 0)));
}

void GetFileNameDlg::OnChangeName()
{
	_changing = true;
	size_t match = 0;
	auto txt = _fileName->GetEditable()->GetText();
	for( int i = 0; i < _files->GetData()->GetItemCount(); ++i )
	{
		auto filename = _files->GetData()->GetItemText(i, 0);
		size_t n = 0;
		while( n < filename.length() && n < txt.length() )
		{
			if( filename[n] != txt[n] ) break;
			++n;
		}
		if( n > match )
		{
			match = n;
			_files->GetList()->SetCurSel(i);
		}
	}
	_changing = false;
}

bool GetFileNameDlg::OnKeyPressed(UI::InputContext &ic, Plat::Key key)
{
	switch( key )
	{
	//case Plat::Key::Up:
	//case Plat::Key::Down:
	//	static_cast<Window *>(_files)->OnKeyPressed(c);
	//	break;
	case Plat::Key::Enter:
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
