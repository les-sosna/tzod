// gui_getfilename.cpp

#include "stdafx.h"
#include "gui_getfilename.h"

#include "Text.h"
#include "List.h"
#include "Button.h"
#include "Edit.h"

#include "GuiManager.h"

#include "fs/FileSystem.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

GetFileNameDlg::GetFileNameDlg(Window *parent, const Params &param)
  : Dialog(parent, 512, 460)
  , _changing(false)
{
	Text *t = new Text(this, GetWidth() / 2, 16, param.title.c_str(), alignTextCT);
	t->SetTexture("font_default");
	t->Resize(t->GetTextureWidth(), t->GetTextureHeight());

	_folder = param.folder;
	_ext = param.extension;
	_files = new List(this, 20, 56, 472, 300);
	std::set<string_t> files;
	if( _folder->EnumAllFiles(files, ("*." + _ext).c_str()) )
	{
		for( std::set<string_t>::iterator it = files.begin(); it != files.end(); ++it )
		{
			it->erase(it->length() - _ext.length() - 1); // cut out the file extension
			int index = _files->AddItem(it->c_str());
		}
	}
	else
	{
		_ASSERT(FALSE); // EnumAllFiles has returned error...
	}
	_files->Sort();
	_files->eventChangeCurSel.bind(&GetFileNameDlg::OnSelect, this);


	new Text(this, 16, 370, "Имя файла", alignTextLT);
	_fileName = new Edit(this, 20, 385, 472);
	_fileName->eventChange.bind(&GetFileNameDlg::OnChangeName, this);

	(new Button(this, 290, 420, "OK"))->eventClick.bind(&GetFileNameDlg::OnOK, this);
	(new Button(this, 400, 420, "Отмена"))->eventClick.bind(&GetFileNameDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_fileName);
}

GetFileNameDlg::~GetFileNameDlg()
{
}

string_t GetFileNameDlg::GetFileName() const
{
	return _fileName->GetText() + "." + _ext;
}

string_t GetFileNameDlg::GetFileTitle() const
{
	return _fileName->GetText();
}

void GetFileNameDlg::OnSelect(int index)
{
	if( _changing || -1 == index ) return;
	_fileName->SetText(_files->GetItemText(index).c_str());
}

void GetFileNameDlg::OnChangeName()
{
	_changing = true;
	size_t match = 0;
	string_t txt = _fileName->GetText();
	for( int i = 0; i < _files->GetSize(); ++i )
	{
		string_t fn = _files->GetItemText(i);
		size_t n = 0;
		while( n < fn.length() && n < txt.length() )
		{
			if( fn[n] != txt[n] ) break;
			++n;
		}
		if( n > match )
		{
			match = n;
			_files->SetCurSel(i, true);
		}
	}
	_changing = false;
}

void GetFileNameDlg::OnRawChar(int c)
{
	switch( c )
	{
	case VK_UP:
	case VK_DOWN:
		static_cast<Window *>(_files)->OnRawChar(c);
		break;
	case VK_RETURN:
		OnOK();
		break;
	default:
		Dialog::OnRawChar(c);
	}
}

void GetFileNameDlg::OnOK()
{
	Close(_resultOK);
}

void GetFileNameDlg::OnCancel()
{
	Close(_resultCancel);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
