#pragma once
#include <ui/Dialog.h>

class LangCache;

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
	NewCampaignDlg(UI::LayoutManager &manager, FS::FileSystem &fs, LangCache &lang);
	~NewCampaignDlg();
	std::function<void(std::string)> eventCampaignSelected;

private:
	void OnOK();
	void OnCancel();

	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	std::shared_ptr<DefaultListBox> _files;
	FS::FileSystem &_fs;
};
