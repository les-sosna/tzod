#include "NewMap.h"
#include "inc/shell/Config.h"
#include <gc/WorldCfg.h>
#include <loc/Language.h>
#include <ui/Button.h>
#include <ui/Edit.h>
#include <ui/GuiManager.h>
#include <ui/Text.h>
#include <algorithm>

NewMapDlg::NewMapDlg(UI::LayoutManager &manager, TextureManager &texman, ConfCache &conf, LangCache &lang)
	: Dialog(manager, texman, 256, 256)
	, _conf(conf)
{
	auto header = UI::Text::Create(this, texman, 128, 20, lang.newmap_title.Get(), alignTextCT);
	header->SetFont(texman, "font_default");

	UI::Text::Create(this, texman, 40, 75, lang.newmap_width.Get(), alignTextLT);
	_width = std::make_shared<UI::Edit>(manager, texman);
	_width->Move(60, 90);
	_width->SetWidth(80);
	_width->SetInt(_conf.ed_width.GetInt());
	AddFront(_width);

	UI::Text::Create(this, texman, 40, 115, lang.newmap_height.Get(), alignTextLT);
	_height = std::make_shared<UI::Edit>(manager, texman);
	_height->Move(60, 130);
	_height->SetWidth(80);
	_height->SetInt(_conf.ed_height.GetInt());
	AddFront(_height);

	UI::Button::Create(this, texman, lang.common_ok.Get(), 20, 200)->eventClick = std::bind(&NewMapDlg::OnOK, this);
	UI::Button::Create(this, texman, lang.common_cancel.Get(), 140, 200)->eventClick = std::bind(&NewMapDlg::OnCancel, this);

	manager.SetFocusWnd(_width);
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
