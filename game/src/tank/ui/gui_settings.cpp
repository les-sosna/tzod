// gui_settings.cpp

#include "stdafx.h"
#include "gui_settings.h"
#include "GuiManager.h"

#include "Text.h"
#include "List.h"
#include "Button.h"
#include "Scroll.h"

#include "config/Config.h"

#include "KeyMapper.h"


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
	_profiles->eventChangeCurSel.bind(&SettingsDlg::OnSelectProfile, this);

	(new Button(this, 16, 144, "Добавить"))->eventClick.bind(&SettingsDlg::OnAddProfile, this);
	_editProfile = new Button(this, 16, 176, "Изменить");
	_editProfile->eventClick.bind(&SettingsDlg::OnEditProfile, this);
	_editProfile->Enable( false );
	_deleteProfile = new Button(this, 16, 208, "Удалить");
	_deleteProfile->eventClick.bind(&SettingsDlg::OnDeleteProfile, this);
	_deleteProfile->Enable( false );

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


	_profiles->SetCurSel(0, true);
	GetManager()->SetFocusWnd(_profiles);
}

SettingsDlg::~SettingsDlg()
{
}

void SettingsDlg::OnAddProfile()
{
}

void SettingsDlg::OnEditProfile()
{
	int i = _profiles->GetCurSel();
	_ASSERT(i >= 0);
	new ControlProfileDlg(this, g_conf.dm_profiles->GetTable(_profiles->GetItemText(i).c_str()));
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
// class ControlProfileDlg

ControlProfileDlg::ControlProfileDlg(Window *parent, ConfVarTable *profile)
  : Dialog(parent, 10, 10, 512, 384)
  , _profile(profile)
{
	_ASSERT(profile);

	_time = 0;
	_activeIndex = -1;
	_skip = false;


	Move((parent->GetWidth() - GetWidth()) * 0.5f, (parent->GetHeight() - GetHeight()) * 0.5f);
	SetEasyMove(true);

	new Text(this,  10, 5, "Действие", alignTextLT);
	new Text(this, 210, 5, "Кнопка", alignTextLT);
	_actions = new List(this, 10, 20, 400, 300);
	_actions->SetTabPos(0, 2);
	_actions->SetTabPos(1, 200);
	_actions->eventClickItem.bind(&ControlProfileDlg::OnSelectAction, this);

	AddAction( "key_forward"      , "Вперед"            );
	AddAction( "key_back"         , "Назад"             );
	AddAction( "key_left"         , "Поворот налево"    );
	AddAction( "key_right"        , "Поворот направо"   );
	AddAction( "key_fire"         , "Огонь!!!"          );
	AddAction( "key_light"        , "Фары вкл/выкл"     );
	AddAction( "key_tower_left"   , "Орудие налево"     );
	AddAction( "key_tower_right"  , "Орудие направо"    );
	AddAction( "key_tower_center" , "Орудие по ценру"   );
	AddAction( "key_pickup"       , "Подобрать предмет" );
	_actions->SetCurSel(0, true);

	(new Button(this, 304, 360, "ОК"))->eventClick.bind(&ControlProfileDlg::OnOK, this);
	(new Button(this, 408, 360, "Отмена"))->eventClick.bind(&ControlProfileDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_actions);
}

ControlProfileDlg::~ControlProfileDlg()
{
}

void ControlProfileDlg::OnSelectAction(int index)
{
	_actions->SetItemText(index, 1, "...");
	_time = 0;
	_activeIndex = index;
	g_pKeyboard->SetCooperativeLevel( g_env.hMainWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	_skip = true;
	SetTimeStep(true);
}

void ControlProfileDlg::AddAction(const char *rawname, const char *display)
{
	int index = _actions->AddItem(display);
	_actions->SetItemData(index, (ULONG_PTR) rawname);
	_actions->SetItemText(index, 1, 
		g_keys->GetName(g_keys->GetCode(_profile->GetStr(rawname)->Get())).c_str());
}

void ControlProfileDlg::OnOK()
{
	Dialog::Close(_resultOK);
}

void ControlProfileDlg::OnCancel()
{
	Dialog::Close(_resultCancel);
}

void ControlProfileDlg::OnTimeStep(float dt)
{
	_time += dt;
	_actions->SetItemText(_activeIndex, 1, fmodf(_time, 0.6f) > 0.3f ? "" : "...");

	for( int k = 0; k < sizeof(g_env.envInputs.keys) / sizeof(g_env.envInputs.keys[0]); ++k )
	{
		if( g_env.envInputs.keys[k] )
		{
			if( _skip )
			{
				return;
			}
			if( DIK_ESCAPE != k )
			{
				_actions->SetItemText(_activeIndex, 1, g_keys->GetName(k).c_str());
			}
			else
			{
				_actions->SetItemText(_activeIndex, 1, 
					g_keys->GetName(
						g_keys->GetCode(
							_profile->GetStr(
								(const char *) _actions->GetItemData(_activeIndex)
							)->Get()
						)
					).c_str()
				);
			}
			g_pKeyboard->SetCooperativeLevel(g_env.hMainWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
			SetTimeStep(false);
		}
	}

	_skip = false;
}

void ControlProfileDlg::OnRawChar(int c)
{
	switch(c)
	{
	case VK_RETURN:
		if( -1 != _actions->GetCurSel() )
		{
			OnSelectAction(_actions->GetCurSel());
		}
		break;
	default:
		Dialog::OnRawChar(c);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
