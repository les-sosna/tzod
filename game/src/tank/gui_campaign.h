// gui_campaign.h

#pragma once

#include "Dialog.h"

namespace UI
{
class ListDataSourceDefault;
class List;
template <class, class> class ListAdapter;

class NewCampaignDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_files;

public:
	NewCampaignDlg(Window *parent);
	~NewCampaignDlg();

protected:
	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
