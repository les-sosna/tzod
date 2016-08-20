#include "NewMap.h"
#include "ConfigBinding.h"
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
	// Title
	auto text = std::make_shared<UI::Text>(manager, texman);
	text->Move(128, 20);
	text->SetText(ConfBind(lang.newmap_title));
	text->SetAlign(alignTextCT);
	text->SetFont(texman, "font_default");
	AddFront(text);

	text = std::make_shared<UI::Text>(manager, texman);
	text->Move(40, 75);
	text->SetText(ConfBind(lang.newmap_width));
	AddFront(text);

	_width = std::make_shared<UI::Edit>(manager, texman);
	_width->Move(60, 90);
	_width->SetWidth(80);
	_width->SetInt(_conf.ed_width.GetInt());
	AddFront(_width);

	text = std::make_shared<UI::Text>(manager, texman);
	text->Move(40, 115);
	text->SetText(ConfBind(lang.newmap_height));
	AddFront(text);

	_height = std::make_shared<UI::Edit>(manager, texman);
	_height->Move(60, 130);
	_height->SetWidth(80);
	_height->SetInt(_conf.ed_height.GetInt());
	AddFront(_height);

	auto btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(ConfBind(lang.common_ok));
	btn->Move(20, 200);
	btn->eventClick = std::bind(&NewMapDlg::OnOK, this);
	AddFront(btn);

	btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(ConfBind(lang.common_cancel));
	btn->Move(140, 200);
	btn->eventClick = std::bind(&NewMapDlg::OnCancel, this);
	AddFront(btn);

	SetFocus(_width);
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
