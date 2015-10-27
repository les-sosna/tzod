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
}

class NewCampaignDlg : public UI::Dialog
{
public:
	NewCampaignDlg(UI::Window *parent, FS::FileSystem &fs);
	~NewCampaignDlg();
	std::function<void(std::string)> eventCampaignSelected;

private:
	void OnOK();
	void OnCancel();

	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	DefaultListBox *_files;
	FS::FileSystem &_fs;
};
