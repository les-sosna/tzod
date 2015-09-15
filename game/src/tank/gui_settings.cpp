// gui_settings.cpp

#include "Config.h"
#include "gui_settings.h"
#include "KeyMapper.h"

#include <video/TextureManager.h>
#include <loc/Language.h>
#include <ui/Text.h>
#include <ui/List.h>
#include <ui/Button.h>
#include <ui/Scroll.h>
#include <ui/Edit.h>
#include <ui/Combo.h>
#include <ui/DataSourceAdapters.h>
#include <ui/GuiManager.h>

#include <GLFW/glfw3.h>
#include <algorithm>
#include <sstream>

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

SettingsDlg::SettingsDlg(Window *parent)
  : Dialog(parent, 512, 296)
{
	SetEasyMove(true);

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.settings_title.Get(), alignTextCT);
	title->SetFont("font_default");


	//
	// profiles
	//

	float x = 24;
	float y = 48;

	y += Text::Create(this, x, y, g_lang.settings_player1.Get(), alignTextLT)->GetHeight() + 2;
	_player1 = ComboBox::Create(this, &_profilesDataSource);
	_player1->Move(x, y);
	_player1->Resize(128);
	_player1->GetList()->Resize(128, 52);
	y += _player1->GetHeight() + 5;

	y += Text::Create(this, 24, y, g_lang.settings_player2.Get(), alignTextLT)->GetHeight() + 2;
	_player2 = ComboBox::Create(this, &_profilesDataSource);
	_player2->Move(x, y);
	_player2->Resize(128);
	_player2->GetList()->Resize(128, 52);
	y += _player2->GetHeight() + 5;

	y += Text::Create(this, x, y, g_lang.settings_profiles.Get(), alignTextLT)->GetHeight() + 2;
	_profiles = List::Create(this, &_profilesDataSource, x, y, 128, 52);
	UpdateProfilesList(); // fill the list before binding OnChangeSel
	_profiles->eventChangeCurSel = std::bind(&SettingsDlg::OnSelectProfile, this, std::placeholders::_1);

	Button::Create(this, g_lang.settings_profile_new.Get(), 40, 184)->eventClick = std::bind(&SettingsDlg::OnAddProfile, this);
	_editProfile = Button::Create(this, g_lang.settings_profile_edit.Get(), 40, 216);
	_editProfile->eventClick = std::bind(&SettingsDlg::OnEditProfile, this);
	_editProfile->SetEnabled( false );
	_deleteProfile = Button::Create(this, g_lang.settings_profile_delete.Get(), 40, 248);
	_deleteProfile->eventClick = std::bind(&SettingsDlg::OnDeleteProfile, this);
	_deleteProfile->SetEnabled( false );


	//
	// other settings
	//

	x = 200;
	y = 48;

	_showFps = CheckBox::Create(this, x, y, g_lang.settings_show_fps.Get());
	_showFps->SetCheck(g_conf.ui_showfps.Get());
	y += _showFps->GetHeight();

	_showTime = CheckBox::Create(this, x, y, g_lang.settings_show_time.Get());
	_showTime->SetCheck(g_conf.ui_showtime.Get());
	y += _showTime->GetHeight();

	_showNames = CheckBox::Create(this, x, y, g_lang.settings_show_names.Get());
	_showNames->SetCheck(g_conf.g_shownames.Get());
	y += _showNames->GetHeight();

	_askDisplaySettings = CheckBox::Create(this, x, y, g_lang.settings_ask_for_display_mode.Get());
	_askDisplaySettings->SetCheck(g_conf.r_askformode.Get());
	y += _askDisplaySettings->GetHeight();

	Text::Create(this, x + 50, y += 20, g_lang.settings_sfx_volume.Get(), alignTextRT);
	_volumeSfx = ScrollBarHorizontal::Create(this, x + 60, y, 150);
	_volumeSfx->SetDocumentSize(1);
	_volumeSfx->SetLineSize(0.1f);
	_volumeSfx->SetPos(expf(g_conf.s_volume.GetFloat() / 2171.0f) - 0.01f);
	_volumeSfx->eventScroll = std::bind(&SettingsDlg::OnVolumeSfx, this, std::placeholders::_1);
	_initialVolumeSfx = g_conf.s_volume.GetInt();

	Text::Create(this, x + 50, y += 20, g_lang.settings_music_volume.Get(), alignTextRT);
	_volumeMusic = ScrollBarHorizontal::Create(this, x + 60, y, 150);
	_volumeMusic->SetDocumentSize(1);
	_volumeMusic->SetLineSize(0.1f);
	_volumeMusic->SetPos(expf(g_conf.s_musicvolume.GetFloat() / 2171.0f) - 0.01f);
	_volumeMusic->eventScroll = std::bind(&SettingsDlg::OnVolumeMusic, this, std::placeholders::_1);
	_initialVolumeMusic = g_conf.s_musicvolume.GetInt();


	//
	// OK & Cancel
	//

	Button::Create(this, g_lang.common_ok.Get(), 304, 256)->eventClick = std::bind(&SettingsDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 408, 256)->eventClick = std::bind(&SettingsDlg::OnCancel, this);


	_profiles->SetCurSel(0, true);
	GetManager().SetFocusWnd(_profiles);
}

SettingsDlg::~SettingsDlg()
{
}

void SettingsDlg::OnVolumeSfx(float pos)
{
	g_conf.s_volume.SetInt( int(2171.0f * logf(0.01f + pos)) );
}

void SettingsDlg::OnVolumeMusic(float pos)
{
	g_conf.s_musicvolume.SetInt( int(2171.0f * logf(0.01f + pos)) );
}

void SettingsDlg::OnAddProfile()
{
	(new ControlProfileDlg(this, nullptr))->eventClose = std::bind(&SettingsDlg::OnProfileEditorClosed, this, std::placeholders::_1);
}

void SettingsDlg::OnEditProfile()
{
	int i = _profiles->GetCurSel();
	assert(i >= 0);
	(new ControlProfileDlg(this, _profilesDataSource.GetItemText(i, 0).c_str()))
		->eventClose = std::bind(&SettingsDlg::OnProfileEditorClosed, this, std::placeholders::_1);
}

void SettingsDlg::OnDeleteProfile()
{
	int i = _profiles->GetCurSel();
	assert(i >= 0);
	if( g_conf.cl_playerinfo.profile.Get() == _profilesDataSource.GetItemText(i, 0) )
	{
		// profile that is being deleted is used in network settings
		g_conf.cl_playerinfo.profile.Set("");
	}
	g_conf.dm_profiles.Remove(_profilesDataSource.GetItemText(i, 0));
	UpdateProfilesList();
}

void SettingsDlg::OnSelectProfile(int index)
{
	_editProfile->SetEnabled( -1 != index );
	_deleteProfile->SetEnabled( -1 != index );
}

void SettingsDlg::OnOK()
{
	g_conf.ui_showfps.Set(_showFps->GetCheck());
	g_conf.ui_showtime.Set(_showTime->GetCheck());
	g_conf.g_shownames.Set(_showNames->GetCheck());
	g_conf.r_askformode.Set(_askDisplaySettings->GetCheck());

	Close(_resultCancel); // return cancel to show back the main menu
}

void SettingsDlg::OnCancel()
{
	g_conf.s_volume.SetInt(_initialVolumeSfx);
	g_conf.s_musicvolume.SetInt(_initialVolumeMusic);
	Close(_resultCancel);
}

void SettingsDlg::UpdateProfilesList()
{
	int sel = _profiles->GetCurSel();
	std::vector<std::string> profiles = g_conf.dm_profiles.GetKeys();
	_profilesDataSource.DeleteAllItems();
	for( size_t i = 0; i < profiles.size(); ++i )
	{
		_profilesDataSource.AddItem(profiles[i]);
	}
	_profiles->SetCurSel(std::min(_profilesDataSource.GetItemCount() - 1, sel));
}

void SettingsDlg::OnProfileEditorClosed(int code)
{
	if( _resultOK == code )
	{
		UpdateProfilesList();
		GetManager().SetFocusWnd(_profiles);

	}
}

///////////////////////////////////////////////////////////////////////////////
// class ControlProfileDlg

static std::string GenerateProfileName()
{
	int i = 0;
	std::ostringstream buf;
	do
	{
		buf.str("");
		buf << g_lang.profile_autoname.Get() << ++i;
	}
	while( g_conf.dm_profiles.Find(buf.str()) );
	return buf.str();
}

ControlProfileDlg::ControlProfileDlg(Window *parent, const char *profileName)
  : Dialog(parent, 448, 416)
  , _nameOrig(profileName ? profileName : GenerateProfileName())
  , _profile(g_conf.dm_profiles.GetTable(_nameOrig))
  , _time(0)
  , _activeIndex(-1)
  , _createNewProfile(!profileName)
{
	SetEasyMove(true);

	Text::Create(this, 20, 15, g_lang.profile_name.Get(), alignTextLT);
	_nameEdit = Edit::Create(this, 20, 30, 250);
	_nameEdit->SetText(_nameOrig);

	Text::Create(this,  20, 65, g_lang.profile_action.Get(), alignTextLT);
	Text::Create(this, 220, 65, g_lang.profile_key.Get(), alignTextLT);
	_actions = DefaultListBox::Create(this);
	_actions->Move(20, 80);
	_actions->Resize(400, 250);
	_actions->SetTabPos(0, 2);
	_actions->SetTabPos(1, 200);
	_actions->eventClickItem = std::bind(&ControlProfileDlg::OnSelectAction, this, std::placeholders::_1);

	AddAction(_profile.key_forward      , g_lang.action_move_forward.Get()  );
	AddAction(_profile.key_back         , g_lang.action_move_backward.Get() );
	AddAction(_profile.key_left         , g_lang.action_turn_left.Get()     );
	AddAction(_profile.key_right        , g_lang.action_turn_right.Get()    );
	AddAction(_profile.key_fire         , g_lang.action_fire.Get()          );
	AddAction(_profile.key_light        , g_lang.action_toggle_lights.Get() );
	AddAction(_profile.key_tower_left   , g_lang.action_tower_left.Get()    );
	AddAction(_profile.key_tower_right  , g_lang.action_tower_right.Get()   );
	AddAction(_profile.key_tower_center , g_lang.action_tower_center.Get()  );
	AddAction(_profile.key_pickup       , g_lang.action_pickup.Get()        );
	_actions->SetCurSel(0, true);

	_aimToMouseChkBox = CheckBox::Create(this, 16, 345, g_lang.profile_mouse_aim.Get());
	_aimToMouseChkBox->SetCheck(_profile.aim_to_mouse.Get());

	_moveToMouseChkBox = CheckBox::Create(this, 146, 345, g_lang.profile_mouse_move.Get());
	_moveToMouseChkBox->SetCheck(_profile.move_to_mouse.Get());

	_arcadeStyleChkBox = CheckBox::Create(this, 276, 345, g_lang.profile_arcade_style.Get());
	_arcadeStyleChkBox->SetCheck(_profile.arcade_style.Get());

	Button::Create(this, g_lang.common_ok.Get(), 240, 380)->eventClick = std::bind(&ControlProfileDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 344, 380)->eventClick = std::bind(&ControlProfileDlg::OnCancel, this);

	GetManager().SetFocusWnd(_actions);
}

ControlProfileDlg::~ControlProfileDlg()
{
}

void ControlProfileDlg::OnSelectAction(int index)
{
	_actions->GetData()->SetItemText(index, 1, "...");
	_time = 0;
	_activeIndex = index;
	SetTimeStep(true);
	GetManager().SetFocusWnd(this);
}

void ControlProfileDlg::AddAction(ConfVarString &var, const std::string &display)
{
	ConfVarTable::KeyListType names = _profile->GetRoot()->GetKeys();
	for( size_t i = 0; i != names.size(); ++i )
	{
		if( _profile->GetRoot()->Find(names[i]) == &var )
		{
			int index = _actions->GetData()->AddItem(display);
			_actions->GetData()->SetItemText(index, 1, GetKeyName(GetKeyCode(var.Get())));
			_actions->GetData()->SetItemData(index, i);
			return;
		}
	}
	assert(false);
}

void ControlProfileDlg::OnOK()
{
	if( _nameEdit->GetText().empty() || !g_conf.dm_profiles.Rename(_profile->GetRoot(), _nameEdit->GetText()) )
	{
		return;
	}

	ConfVarTable::KeyListType names = _profile->GetRoot()->GetKeys();
	for( int i = 0; i < _actions->GetData()->GetItemCount(); ++i )
	{
		_profile->GetRoot()->SetStr(names[_actions->GetData()->GetItemData(i)], _actions->GetData()->GetItemText(i, 1));
	}

	_profile.aim_to_mouse.Set(_aimToMouseChkBox->GetCheck());
	_profile.move_to_mouse.Set(_moveToMouseChkBox->GetCheck());
	_profile.arcade_style.Set(_arcadeStyleChkBox->GetCheck());

	Close(_resultOK);
}

void ControlProfileDlg::OnCancel()
{
	if( _createNewProfile )
	{
		g_conf.dm_profiles.Remove(_profile->GetRoot());
	}
	Close(_resultCancel);
}

void ControlProfileDlg::OnTimeStep(float dt)
{
	_time += dt;
	_actions->GetData()->SetItemText(_activeIndex, 1, fmodf(_time, 0.6f) > 0.3f ? "" : "...");
}

bool ControlProfileDlg::OnRawChar(int c)
{
	if (-1 != _activeIndex)
	{
		if (GLFW_KEY_ESCAPE == c)
		{
			int oldKeyCode = GetKeyCode(_profile->GetRoot()->GetStr((const char *) _actions->GetData()->GetItemData(_activeIndex), "")->Get());
			_actions->GetData()->SetItemText(_activeIndex, 1, GetKeyName(oldKeyCode));
		}
		else
		{
			_actions->GetData()->SetItemText(_activeIndex, 1, GetKeyName(c));
		}
		SetTimeStep(false);
		_activeIndex = -1;
		GetManager().SetFocusWnd(_actions);
	}
	else
	{
		switch( c )
		{
		case GLFW_KEY_ENTER:
			if( GetManager().GetFocusWnd() == _actions && -1 != _actions->GetCurSel() )
			{
				OnSelectAction(_actions->GetCurSel());
			}
			else
			{
				OnOK();
			}
			break;
		case GLFW_KEY_ESCAPE:
			break;
		default:
			return Dialog::OnRawChar(c);
		}
	}
	return true;
}

} // end of namespace UI

