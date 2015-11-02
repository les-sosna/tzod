#include "NewMap.h"
#include "inc/shell/Config.h"
#include <gc/WorldCfg.h>
#include <loc/Language.h>
#include <ui/Button.h>
#include <ui/Edit.h>
#include <ui/GuiManager.h>
#include <ui/Text.h>
#include <algorithm>

NewMapDlg::NewMapDlg(Window *parent, ConfCache &conf, LangCache &lang)
	: Dialog(parent, 256, 256)
	, _conf(conf)
{
	UI::Text *header = UI::Text::Create(this, 128, 20, lang.newmap_title.Get(), alignTextCT);
	header->SetFont("font_default");

	UI::Text::Create(this, 40, 75, lang.newmap_width.Get(), alignTextLT);
	_width = UI::Edit::Create(this, 60, 90, 80);
	_width->SetInt(_conf.ed_width.GetInt());

	UI::Text::Create(this, 40, 115, lang.newmap_height.Get(), alignTextLT);
	_height = UI::Edit::Create(this, 60, 130, 80);
	_height->SetInt(_conf.ed_height.GetInt());

	UI::Button::Create(this, lang.common_ok.Get(), 20, 200)->eventClick = std::bind(&NewMapDlg::OnOK, this);
	UI::Button::Create(this, lang.common_cancel.Get(), 140, 200)->eventClick = std::bind(&NewMapDlg::OnCancel, this);

	GetManager().SetFocusWnd(_width);
}

void NewMapDlg::OnOK()
{
	_conf.ed_width.SetInt(std::max(LEVEL_MINSIZE, std::min(LEVEL_MAXSIZE, _width->GetInt())));
	_conf.ed_height.SetInt(std::max(LEVEL_MINSIZE, std::min(LEVEL_MAXSIZE, _height->GetInt())));

	Close(_resultOK);
}

void NewMapDlg::OnCancel()
{
	Close(_resultCancel);
}
