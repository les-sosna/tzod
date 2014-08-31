// gui_getfilename.h

#pragma once

#include "core/SafePtr.h"
#include <ui/Dialog.h>
#include <string>
#include <memory>

namespace FS
{
	class FileSystem;
}

namespace UI
{
class ListDataSourceDefault;
class List;
class Edit;
template<class, class> class ListAdapter;

class GetFileNameDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_files;
	Edit *_fileName;
	std::string _ext;
	std::shared_ptr<FS::FileSystem> _folder;

	bool _changing;

public:
	struct Params
	{
		std::string title;
		std::string extension;
		std::shared_ptr<FS::FileSystem> folder;
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
