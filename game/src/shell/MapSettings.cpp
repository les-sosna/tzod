#include "MapSettings.h"
#include <gv/ThemeManager.h>
#include <gc/World.h>
#include <loc/Language.h>
#include <ui/Button.h>
#include <ui/Combo.h>
#include <ui/Edit.h>
#include <ui/DataSourceAdapters.h>
#include <ui/Text.h>
#include <ui/List.h>

static size_t FindTheme(const ThemeManager &themeManager, const std::string &name)
{
	for (size_t i = 0; i < themeManager.GetThemeCount(); ++i)
	{
		if (themeManager.GetThemeName(i) == name)
		{
			return i;
		}
	}
	return 0;
}

MapSettingsDlg::MapSettingsDlg(UI::LayoutManager &manager, TextureManager &texman, World &world/*, const ThemeManager &themeManager*/, LangCache &lang)
	: Dialog(manager, texman, 512, 512)
	, _world(world)
{
	SetEasyMove(true);

	auto title = UI::Text::Create(this, texman, GetWidth() / 2, 16, lang.map_title.Get(), alignTextCT);
	title->SetFont(texman, "font_default");


	float x1 = 20;
	float x2 = x1 + 12;

	float y = 32;

	UI::Text::Create(this, texman, x1, y += 20, lang.map_author.Get(), alignTextLT);
	_author = std::make_shared<UI::Edit>(manager, texman);
	_author->Move(x2, y += 15);
	_author->SetWidth(256);
	_author->SetText(texman, world._infoAuthor);
	AddFront(_author);

	UI::Text::Create(this, texman, x1, y += 20, lang.map_email.Get(), alignTextLT);
	_email = std::make_shared<UI::Edit>(manager, texman);
	_email->Move(x2, y += 15);
	_email->SetWidth(256);
	_email->SetText(texman, world._infoEmail);
	AddFront(_email);

	UI::Text::Create(this, texman, x1, y += 20, lang.map_url.Get(), alignTextLT);
	_url = std::make_shared<UI::Edit>(manager, texman);
	_url->Move(x2, y += 15);
	_url->SetWidth(256);
	_url->SetText(texman, world._infoUrl);
	AddFront(_url);

	UI::Text::Create(this, texman, x1, y += 20, lang.map_desc.Get(), alignTextLT);
	_desc = std::make_shared<UI::Edit>(manager, texman);
	_desc->Move(x2, y += 15);
	_desc->SetWidth(256);
	_desc->SetText(texman, world._infoDesc);
	AddFront(_url);

	UI::Text::Create(this, texman, x1, y += 20, lang.map_init_script.Get(), alignTextLT);
	_onInit = std::make_shared<UI::Edit>(manager, texman);
	_onInit->Move(x2, y += 15);
	_onInit->SetWidth(256);
	_onInit->SetText(texman, world._infoOnInit);
	AddFront(_onInit);

	UI::Text::Create(this, texman, x1, y += 20, lang.map_theme.Get(), alignTextLT);
	_theme = DefaultComboBox::Create(this, texman);
	_theme->Move(x2, y += 15);
	_theme->Resize(256);
/*	for (size_t i = 0; i < themeManager.GetThemeCount(); i++)
	{
		_theme->GetData()->AddItem(themeManager.GetThemeName(i));
	}
	_theme->SetCurSel(FindTheme(themeManager, world._infoTheme));*/
	_theme->GetList()->AlignHeightToContent();


	//
	// OK & Cancel
	//
	UI::Button::Create(this, texman, lang.common_ok.Get(), 304, 480)->eventClick = std::bind(&MapSettingsDlg::OnOK, this);
	UI::Button::Create(this, texman, lang.common_cancel.Get(), 408, 480)->eventClick = std::bind(&MapSettingsDlg::OnCancel, this);
}

MapSettingsDlg::~MapSettingsDlg()
{
}

void MapSettingsDlg::OnOK()
{
	_world._infoAuthor = _author->GetText();
	_world._infoEmail = _email->GetText();
	_world._infoUrl = _url->GetText();
	_world._infoDesc = _desc->GetText();
	_world._infoOnInit = _onInit->GetText();

	int i = _theme->GetCurSel();
	if (0 != i)
	{
		_world._infoTheme = _theme->GetData()->GetItemText(i, 0);
	}
	else
	{
		_world._infoTheme.clear();
	}

	Close(_resultOK);
}

void MapSettingsDlg::OnCancel()
{
	Close(_resultCancel);
}
