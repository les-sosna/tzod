// gui_campaign.h

#pragma once

#include <ui/Dialog.h>

namespace FS
{
	class FileSystem;
}

namespace UI
{
class ListDataSourceDefault;
class List;
template <class, class> class ListAdapter;

class NewCampaignDlg : public Dialog
{
public:
	NewCampaignDlg(Window *parent, FS::FileSystem &fs);
	~NewCampaignDlg();

private:
	void OnOK();
	void OnCancel();

	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_files;
	FS::FileSystem &_fs;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
