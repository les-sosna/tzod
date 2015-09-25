#include "gui_campaign.h"
#include "gui_desktop.h"
#include "Config.h"

#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <ui/List.h>
#include <ui/Button.h>
#include <ui/Text.h>
#include <ui/DataSourceAdapters.h>
#include <ui/GuiManager.h>


NewCampaignDlg::NewCampaignDlg(UI::Window *parent, FS::FileSystem &fs)
  : UI::Dialog(parent, 512, 400)
  , _fs(fs)
{
	UI::Text *t = UI::Text::Create(this, GetWidth() / 2, 16, g_lang.campaign_title.Get(), alignTextCT);
	t->SetFont("font_default");

	_files = DefaultListBox::Create(this);
	_files->Move(20, 56);
	_files->Resize(472, 280);

	auto files = _fs.GetFileSystem("campaign")->EnumAllFiles("*.lua");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		it->erase(it->length() - 4); // cut out the file extension
		_files->GetData()->AddItem(*it);
	}
	_files->GetData()->Sort();

	UI::Button::Create(this, g_lang.campaign_ok.Get(), 290, 360)->eventClick = std::bind(&NewCampaignDlg::OnOK, this);
	UI::Button::Create(this, g_lang.campaign_cancel.Get(), 400, 360)->eventClick = std::bind(&NewCampaignDlg::OnCancel, this);
}

NewCampaignDlg::~NewCampaignDlg()
{
}

void NewCampaignDlg::OnOK()
{
	if( -1 != _files->GetCurSel() )
	{
		if( eventCampaignSelected )
			eventCampaignSelected(_files->GetData()->GetItemText(_files->GetCurSel(), 0));
	}
}

void NewCampaignDlg::OnCancel()
{
	if( eventCampaignSelected )
		eventCampaignSelected(std::string());
}
