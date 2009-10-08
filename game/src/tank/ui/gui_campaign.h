// gui_campaign.h

#pragma once

#include "Base.h"
#include "Dialog.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

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
