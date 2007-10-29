// gui_getfilename.h

#pragma once

#include "Base.h"
#include "Dialog.h"


// forward declarations
class IFileSystem;

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class GetFileNameDlg : public Dialog
{
	List *_files;
	Edit *_fileName;
	string_t _ext;
	SafePtr<IFileSystem> _folder;

	bool _changing;

public:
	struct Params
	{
		string_t title;
		string_t extension;
		SafePtr<IFileSystem> folder;
	};

	GetFileNameDlg(Window *parent, const Params &param);
	~GetFileNameDlg();

	string_t GetFileName() const;
	string_t GetFileTitle() const;

protected:
	void OnSelect(int index);
	void OnChangeName();
	void OnRawChar(int c);

	void OnOK();
	void OnCancel();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
