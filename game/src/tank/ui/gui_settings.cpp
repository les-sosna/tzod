// gui_settings.cpp

#include "stdafx.h"
#include "gui_settings.h"

#include "Text.h"
#include "List.h"
#include "Button.h"
#include "Scroll.h"

#include "config/Config.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

SettingsDlg::SettingsDlg(Window *parent) : Dialog(parent, 0, 0, 512, 256)
{
	Move((parent->GetWidth() - GetWidth()) * 0.5f, (parent->GetHeight() - GetHeight()) * 0.5f);
	SetEasyMove(true);


	//
	// profiles
	//

	new Text(this, 8, 8, "Профили", alignTextLT);
	_profiles = new List(this, 8, 24, 128, 104);
	std::vector<string_t> profiles;
	g_conf.dm_profiles->GetKeyList(profiles);
	for( size_t i = 0; i < profiles.size(); ++i )
	{
		int index = _profiles->AddItem(profiles[i].c_str());
	}

	(new Button(this, 16, 144, "Добавить"))->eventClick.bind(&SettingsDlg::OnAddProfile, this);
	_editProfile = new Button(this, 16, 176, "Изменить");
	_editProfile->eventClick.bind(&SettingsDlg::OnEditProfile, this);
	_deleteProfile = new Button(this, 16, 208, "Удалить");
	_deleteProfile->eventClick.bind(&SettingsDlg::OnDeleteProfile, this);


	//
	// other settings
	//

	float x = 192;
	float y = 8;

	_showFps = new CheckBox(this, x, y, "Показать FPS");
	_showFps->SetCheck(g_conf.ui_showfps->Get());
	y += _showFps->GetHeight();

	_showTime = new CheckBox(this, x, y, "Показать время");
	_showTime->SetCheck(g_conf.ui_showtime->Get());
	y += _showTime->GetHeight();

	_particles = new CheckBox(this, x, y, "Частицы");
	_particles->SetCheck(g_conf.g_particles->Get());
	y += _particles->GetHeight();

	_showDamage = new CheckBox(this, x, y, "Показывать повреждение");
	_showDamage->SetCheck(g_conf.g_showdamage->Get());
	y += _showDamage->GetHeight();


//	_volume = new ScrollBar(this, x + 192, 8, 128 );
//	_volume->SetLimit(100);


	//
	// OK & Cancel
	//

	(new Button(this, 304, 216, "ОК"))->eventClick.bind(&SettingsDlg::OnOK, this);
	(new Button(this, 408, 216, "Отмена"))->eventClick.bind(&SettingsDlg::OnCancel, this);
}

SettingsDlg::~SettingsDlg()
{
}

void SettingsDlg::OnAddProfile()
{

}

void SettingsDlg::OnEditProfile()
{

}

void SettingsDlg::OnDeleteProfile()
{
	
}

void SettingsDlg::OnSelectProfile(int index)
{
	_editProfile->Enable( -1 != index );
	_deleteProfile->Enable( -1 != index );
}

void SettingsDlg::OnOK()
{
	g_conf.ui_showfps->Set(_showFps->GetCheck());
	g_conf.ui_showtime->Set(_showTime->GetCheck());
	g_conf.g_particles->Set(_particles->GetCheck());
	g_conf.g_showdamage->Set(_showDamage->GetCheck());

	Close(_resultOK);
}

void SettingsDlg::OnCancel()
{
	Close(_resultCancel);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
