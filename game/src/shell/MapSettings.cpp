#include "MapSettings.h"
#include <app/ThemeManager.h>
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

MapSettingsDlg::MapSettingsDlg(Window *parent, World &world, const ThemeManager &themeManager)
	: Dialog(parent, 512, 512)
	, _world(world)
{
	SetEasyMove(true);

	UI::Text *title = UI::Text::Create(this, GetWidth() / 2, 16, g_lang.map_title.Get(), alignTextCT);
	title->SetFont("font_default");


	float x1 = 20;
	float x2 = x1 + 12;

	float y = 32;

	UI::Text::Create(this, x1, y += 20, g_lang.map_author.Get(), alignTextLT);
	_author = UI::Edit::Create(this, x2, y += 15, 256);
	_author->SetText(world._infoAuthor);

	UI::Text::Create(this, x1, y += 20, g_lang.map_email.Get(), alignTextLT);
	_email = UI::Edit::Create(this, x2, y += 15, 256);
	_email->SetText(world._infoEmail);

	UI::Text::Create(this, x1, y += 20, g_lang.map_url.Get(), alignTextLT);
	_url = UI::Edit::Create(this, x2, y += 15, 256);
	_url->SetText(world._infoUrl);

	UI::Text::Create(this, x1, y += 20, g_lang.map_desc.Get(), alignTextLT);
	_desc = UI::Edit::Create(this, x2, y += 15, 256);
	_desc->SetText(world._infoDesc);

	UI::Text::Create(this, x1, y += 20, g_lang.map_init_script.Get(), alignTextLT);
	_onInit = UI::Edit::Create(this, x2, y += 15, 256);
	_onInit->SetText(world._infoOnInit);

	UI::Text::Create(this, x1, y += 20, g_lang.map_theme.Get(), alignTextLT);
	_theme = DefaultComboBox::Create(this);
	_theme->Move(x2, y += 15);
	_theme->Resize(256);
	for (size_t i = 0; i < themeManager.GetThemeCount(); i++)
	{
		_theme->GetData()->AddItem(themeManager.GetThemeName(i));
	}
	_theme->SetCurSel(FindTheme(themeManager, world._infoTheme));
	_theme->GetList()->AlignHeightToContent();


	//
	// OK & Cancel
	//
	UI::Button::Create(this, g_lang.common_ok.Get(), 304, 480)->eventClick = std::bind(&MapSettingsDlg::OnOK, this);
	UI::Button::Create(this, g_lang.common_cancel.Get(), 408, 480)->eventClick = std::bind(&MapSettingsDlg::OnCancel, this);
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
