// gui_getfilename.h

#pragma once

#include "Base.h"
#include "Dialog.h"
#include "core/SafePtr.h"

#include <string>

namespace FS
{
	class FileSystem;
}

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class GetFileNameDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_files;
	Edit *_fileName;
	std::string _ext;
	SafePtr<FS::FileSystem> _folder;

	bool _changing;

public:
	struct Params
	{
		std::string title;
		std::string extension;
		SafePtr<FS::FileSystem> folder;
	};

	GetFileNameDlg(Window *parent, const Params &param);
	virtual ~GetFileNameDlg();

	std::string GetFileName() const;
	std::string GetFileTitle() const;

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
