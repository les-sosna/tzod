#include "Campaign.h"
#include "inc/shell/Config.h"

#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <ui/List.h>
#include <ui/Button.h>
#include <ui/Text.h>
#include <ui/DataSourceAdapters.h>
#include <ui/GuiManager.h>


NewCampaignDlg::NewCampaignDlg(UI::LayoutManager &manager, TextureManager &texman, FS::FileSystem &fs, LangCache &lang)
  : UI::Dialog(manager, texman, 512, 400)
  , _fs(fs)
{
	auto t = UI::Text::Create(this, texman, GetWidth() / 2, 16, lang.campaign_title.Get(), alignTextCT);
	t->SetFont(texman, "font_default");

	_files = std::make_shared<DefaultListBox>(manager, texman);
	_files->Move(20, 56);
	_files->Resize(472, 280);
	AddFront(_files);

	auto files = _fs.GetFileSystem("campaign")->EnumAllFiles("*.lua");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		it->erase(it->length() - 4); // cut out the file extension
		_files->GetData()->AddItem(*it);
	}
	_files->GetData()->Sort();

	UI::Button::Create(this, texman, lang.campaign_ok.Get(), 290, 360)->eventClick = std::bind(&NewCampaignDlg::OnOK, this);
	UI::Button::Create(this, texman, lang.campaign_cancel.Get(), 400, 360)->eventClick = std::bind(&NewCampaignDlg::OnCancel, this);
}

NewCampaignDlg::~NewCampaignDlg()
{
}

void NewCampaignDlg::OnOK()
{
	if( -1 != _files->GetCurSel() )
	{
		if( eventCampaignSelected )
			eventCampaignSelected(std::static_pointer_cast<NewCampaignDlg>(shared_from_this()), _files->GetData()->GetItemText(_files->GetCurSel(), 0));
	}
}

void NewCampaignDlg::OnCancel()
{
	if( eventCampaignSelected )
		eventCampaignSelected(std::static_pointer_cast<NewCampaignDlg>(shared_from_this()), std::string());
}
