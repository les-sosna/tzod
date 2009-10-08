// gui_getfilename.h

#pragma once

#include "Base.h"
#include "Dialog.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class GetFileNameDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_files;
	Edit *_fileName;
	string_t _ext;
	SafePtr<FS::FileSystem> _folder;

	bool _changing;

public:
	struct Params
	{
		string_t title;
		string_t extension;
		SafePtr<FS::FileSystem> folder;
	};

	GetFileNameDlg(Window *parent, const Params &param);
	virtual ~GetFileNameDlg();

	string_t GetFileName() const;
	string_t GetFileTitle() const;

protected:
	void OnSelect(int index);
	void OnChangeName();
	bool OnRawChar(int c);

	void OnOK();
	void OnCancel();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
