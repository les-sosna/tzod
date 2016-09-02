#include "Campaign.h"
#include "ConfigBinding.h"
#include "inc/shell/Config.h"

#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <ui/List.h>
#include <ui/Button.h>
#include <ui/Text.h>
#include <ui/DataSourceAdapters.h>
#include <ui/GuiManager.h>


NewCampaignDlg::NewCampaignDlg(UI::LayoutManager &manager, TextureManager &texman, FS::FileSystem &fs, LangCache &lang)
  : UI::Dialog(manager, texman)
  , _fs(fs)
{
    Resize(512, 400);
    
	auto t = std::make_shared<UI::Text>(manager, texman);
	t->Move(GetWidth() / 2, 16);
	t->SetText(ConfBind(lang.campaign_title));
	t->SetAlign(alignTextCT);
	t->SetFont(texman, "font_default");
	AddFront(t);

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

	auto btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(ConfBind(lang.campaign_ok));
	btn->Move(290, 360);
	btn->eventClick = std::bind(&NewCampaignDlg::OnOK, this);
	AddFront(btn);

	btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(ConfBind(lang.campaign_cancel));
	btn->Move(400, 360);
	btn->eventClick = std::bind(&NewCampaignDlg::OnCancel, this);
	AddFront(btn);
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
