// gui_campaign.cpp

#include "globals.h"
#include "gui_campaign.h"
#include "gui_desktop.h"

#include "List.h"
#include "Button.h"
#include "Text.h"
#include "DataSourceAdapters.h"

#include "GuiManager.h"

#include "functions.h"
#include "script.h"

#include "fs/FileSystem.h"
#include "config/Config.h"
#include "config/Language.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

NewCampaignDlg::NewCampaignDlg(Window *parent)
  : Dialog(parent, 512, 400)
{
	PauseGame(true);

	Text *t = Text::Create(this, GetWidth() / 2, 16, g_lang.campaign_title.Get(), alignTextCT);
	t->SetFont("font_default");

	_files = DefaultListBox::Create(this);
	_files->Move(20, 56);
	_files->Resize(472, 280);

	auto files = g_fs->GetFileSystem("campaign")->EnumAllFiles("*.lua");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		it->erase(it->length() - 4); // cut out the file extension
		_files->GetData()->AddItem(*it);
	}
	_files->GetData()->Sort();

	Button::Create(this, g_lang.campaign_ok.Get(), 290, 360)->eventClick = std::bind(&NewCampaignDlg::OnOK, this);
	Button::Create(this, g_lang.campaign_cancel.Get(), 400, 360)->eventClick = std::bind(&NewCampaignDlg::OnCancel, this);
}

NewCampaignDlg::~NewCampaignDlg()
{
	PauseGame(false);
}

void NewCampaignDlg::OnOK()
{
	if( -1 == _files->GetCurSel() )
	{
		return;
	}

	g_conf.ui_showmsg.Set(true);

	const std::string& name = _files->GetData()->GetItemText(_files->GetCurSel(), 0);
	if( !script_exec_file(g_env.L, ("campaign/" + name + ".lua").c_str()) )
	{
		static_cast<Desktop*>(GetManager()->GetDesktop())->ShowConsole(true);
	}

	Close(_resultOK);
}

void NewCampaignDlg::OnCancel()
{
	Close(_resultCancel);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

