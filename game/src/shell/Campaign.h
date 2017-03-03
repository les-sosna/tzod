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
	NewCampaignDlg(FS::FileSystem &fs, LangCache &lang);
	~NewCampaignDlg();
	std::function<void(std::shared_ptr<NewCampaignDlg>, std::string)> eventCampaignSelected;

private:
	void OnOK();

	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	std::shared_ptr<DefaultListBox> _files;
	FS::FileSystem &_fs;
};
