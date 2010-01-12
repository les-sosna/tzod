// gui_settings.cpp

#include "stdafx.h"
#include "gui_settings.h"
#include "GuiManager.h"

#include "Text.h"
#include "List.h"
#include "Button.h"
#include "Scroll.h"
#include "Edit.h"
#include "Combo.h"
#include "DataSourceAdapters.h"

#include "config/Config.h"
#include "config/Language.h"

#include "video/TextureManager.h"

#include "KeyMapper.h"
#include "Level.h"

#include "functions.h"
#include "Macros.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

SettingsDlg::SettingsDlg(Window *parent)
  : Dialog(parent, 512, 296)
{
	PauseGame(true);

	SetEasyMove(true);

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.settings_title.Get(), alignTextCT);
	title->SetFont("font_default");


	//
	// profiles
	//

	Text::Create(this, 24, 48, g_lang.settings_profiles.Get(), alignTextLT);
	_profiles = DefaultListBox::Create(this);
	_profiles->Move(24, 64);
	_profiles->Resize(128, 104);
	UpdateProfilesList(); // fill the list before binding OnChangeSel
	_profiles->eventChangeCurSel.bind(&SettingsDlg::OnSelectProfile, this);

	Button::Create(this, g_lang.settings_profile_new.Get(), 40, 184)->eventClick.bind(&SettingsDlg::OnAddProfile, this);
	_editProfile = Button::Create(this, g_lang.settings_profile_edit.Get(), 40, 216);
	_editProfile->eventClick.bind(&SettingsDlg::OnEditProfile, this);
	_editProfile->SetEnabled( false );
	_deleteProfile = Button::Create(this, g_lang.settings_profile_delete.Get(), 40, 248);
	_deleteProfile->eventClick.bind(&SettingsDlg::OnDeleteProfile, this);
	_deleteProfile->SetEnabled( false );


	//
	// other settings
	//

	float x = 200;
	float y = 48;

	_showFps = CheckBox::Create(this, x, y, g_lang.settings_show_fps.Get());
	_showFps->SetCheck(g_conf.ui_showfps.Get());
	y += _showFps->GetHeight();

	_showTime = CheckBox::Create(this, x, y, g_lang.settings_show_time.Get());
	_showTime->SetCheck(g_conf.ui_showtime.Get());
	y += _showTime->GetHeight();

	_particles = CheckBox::Create(this, x, y, g_lang.settings_show_particles.Get());
	_particles->SetCheck(g_conf.g_particles.Get());
	y += _particles->GetHeight();

	_showDamage = CheckBox::Create(this, x, y, g_lang.settings_show_damage.Get());
	_showDamage->SetCheck(g_conf.g_showdamage.Get());
	y += _showDamage->GetHeight();

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
	_volumeSfx->eventScroll.bind(&SettingsDlg::OnVolumeSfx, this);
	_initialVolumeSfx = g_conf.s_volume.GetInt();

	Text::Create(this, x + 50, y += 20, g_lang.settings_music_volume.Get(), alignTextRT);
	_volumeMusic = ScrollBarHorizontal::Create(this, x + 60, y, 150);
	_volumeMusic->SetDocumentSize(1);
	_volumeMusic->SetLineSize(0.1f);
	_volumeMusic->SetPos(expf(g_conf.s_musicvolume.GetFloat() / 2171.0f) - 0.01f);
	_volumeMusic->eventScroll.bind(&SettingsDlg::OnVolumeMusic, this);
	_initialVolumeMusic = g_conf.s_musicvolume.GetInt();


	//
	// OK & Cancel
	//

	Button::Create(this, g_lang.common_ok.Get(), 304, 256)->eventClick.bind(&SettingsDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 408, 256)->eventClick.bind(&SettingsDlg::OnCancel, this);


	_profiles->SetCurSel(0, true);
	GetManager()->SetFocusWnd(_profiles);
}

SettingsDlg::~SettingsDlg()
{
	PauseGame(false);
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
	(new ControlProfileDlg(this, NULL))->eventClose = std::tr1::bind(&SettingsDlg::OnProfileEditorClosed, this, _1);
}

void SettingsDlg::OnEditProfile()
{
	int i = _profiles->GetCurSel();
	assert(i >= 0);
	(new ControlProfileDlg(this, _profiles->GetData()->GetItemText(i, 0).c_str()))
		->eventClose = std::tr1::bind(&SettingsDlg::OnProfileEditorClosed, this, _1);
}

void SettingsDlg::OnDeleteProfile()
{
	int i = _profiles->GetCurSel();
	assert(i >= 0);
	if( g_conf.cl_playerinfo.profile.Get() == _profiles->GetData()->GetItemText(i, 0) )
	{
		// profile that is being deleted is used in network settings
		g_conf.cl_playerinfo.profile.Set("");
	}
	g_conf.dm_profiles.Remove(_profiles->GetData()->GetItemText(i, 0));
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
	g_conf.g_particles.Set(_particles->GetCheck());
	g_conf.g_showdamage.Set(_showDamage->GetCheck());
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
	std::vector<string_t> profiles;
	g_conf.dm_profiles.GetKeyList(profiles);
	_profiles->GetData()->DeleteAllItems();
	for( size_t i = 0; i < profiles.size(); ++i )
	{
		int index = _profiles->GetData()->AddItem(profiles[i]);
	}
	_profiles->SetCurSel(__min(_profiles->GetData()->GetItemCount() - 1, sel));
}

void SettingsDlg::OnProfileEditorClosed(int code)
{
	if( _resultOK == code )
	{
		UpdateProfilesList();
		GetManager()->SetFocusWnd(_profiles);

		FOREACH(g_level->GetList(LIST_players), GC_Object, player)
		{
			player->GetProperties()->Exchange(true);
		}
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
  , _skipNextKey(false)
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
	_actions->eventClickItem.bind(&ControlProfileDlg::OnSelectAction, this);

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

	Button::Create(this, g_lang.common_ok.Get(), 240, 380)->eventClick.bind(&ControlProfileDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 344, 380)->eventClick.bind(&ControlProfileDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_actions);
}

ControlProfileDlg::~ControlProfileDlg()
{
}

void ControlProfileDlg::OnSelectAction(int index)
{
	_actions->GetData()->SetItemText(index, 1, "...");
	_time = 0;
	_activeIndex = index;
//	g_pKeyboard->SetCooperativeLevel( g_env.hMainWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	_skipNextKey = true;
	SetTimeStep(true);
}

void ControlProfileDlg::AddAction(ConfVarString &var, const string_t &display)
{
	ConfVarTable::KeyListType names;
	_profile->GetRoot()->GetKeyList(names);
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

	ConfVarTable::KeyListType names;
	_profile->GetRoot()->GetKeyList(names);
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

	for( int k = 0; k < sizeof(g_env.envInputs._keys) / sizeof(g_env.envInputs._keys[0]); ++k )
	{
		if( g_env.envInputs.IsKeyPressed(k) )
		{
			if( _skipNextKey )
			{
				return;
			}
			if( DIK_ESCAPE != k )
			{
				_actions->GetData()->SetItemText(_activeIndex, 1, GetKeyName(k));
			}
			else
			{
				_actions->GetData()->SetItemText(_activeIndex, 1, GetKeyName(GetKeyCode(
					_profile->GetRoot()->GetStr((const char *) _actions->GetData()->GetItemData(_activeIndex), "")->Get())) );
			}
//			g_pKeyboard->SetCooperativeLevel(g_env.hMainWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
			SetTimeStep(false);
		}
	}

	_skipNextKey = false;
}

bool ControlProfileDlg::OnRawChar(int c)
{
	switch( c )
	{
	case VK_RETURN:
		if( GetManager()->GetFocusWnd() == _actions && -1 != _actions->GetCurSel() )
		{
			OnSelectAction(_actions->GetCurSel());
		}
		else
		{
			OnOK();
		}
		break;
	case VK_ESCAPE:
		break;
	default:
		return __super::OnRawChar(c);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

MapSettingsDlg::MapSettingsDlg(Window *parent)
  : Dialog(parent, 512, 512)
{
	SetEasyMove(true);
	assert(g_level);

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.map_title.Get(), alignTextCT);
	title->SetFont("font_default");


	float x1 = 20;
	float x2 = x1 + 12;

	float y = 32;

	Text::Create(this, x1, y += 20, g_lang.map_author.Get(), alignTextLT);
	_author = Edit::Create(this, x2, y += 15, 256);
	_author->SetText(g_level->_infoAuthor);

	Text::Create(this, x1, y += 20, g_lang.map_email.Get(), alignTextLT);
	_email = Edit::Create(this, x2, y += 15, 256);
	_email->SetText(g_level->_infoEmail);

	Text::Create(this, x1, y += 20, g_lang.map_url.Get(), alignTextLT);
	_url = Edit::Create(this, x2, y += 15, 256);
	_url->SetText(g_level->_infoUrl);

	Text::Create(this, x1, y += 20, g_lang.map_desc.Get(), alignTextLT);
	_desc = Edit::Create(this, x2, y += 15, 256);
	_desc->SetText(g_level->_infoDesc);

	Text::Create(this, x1, y += 20, g_lang.map_init_script.Get(), alignTextLT);
	_onInit = Edit::Create(this, x2, y += 15, 256);
	_onInit->SetText(g_level->_infoOnInit);

	Text::Create(this, x1, y += 20, g_lang.map_theme.Get(), alignTextLT);
	_theme = DefaultComboBox::Create(this);
	_theme->Move(x2, y += 15);
	_theme->Resize(256);
	for( size_t i = 0; i < _ThemeManager::Inst().GetThemeCount(); i++ )
	{
		_theme->GetData()->AddItem(_ThemeManager::Inst().GetThemeName(i));
	}
	_theme->SetCurSel(_ThemeManager::Inst().FindTheme(g_level->_infoTheme));
	_theme->GetList()->AlignHeightToContent();


	//
	// OK & Cancel
	//
	Button::Create(this, g_lang.common_ok.Get(), 304, 480)->eventClick.bind(&MapSettingsDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 408, 480)->eventClick.bind(&MapSettingsDlg::OnCancel, this);
}

MapSettingsDlg::~MapSettingsDlg()
{
}

void MapSettingsDlg::OnOK()
{
	g_level->_infoAuthor = _author->GetText();
	g_level->_infoEmail = _email->GetText();
	g_level->_infoUrl = _url->GetText();
	g_level->_infoDesc = _desc->GetText();
	g_level->_infoOnInit = _onInit->GetText();

	int i = _theme->GetCurSel();
	if( 0 != i )
	{
		g_level->_infoTheme = _theme->GetData()->GetItemText(i, 0);
	}
	else
	{
		g_level->_infoTheme.clear();
	}
	if( !_ThemeManager::Inst().ApplyTheme(i) )
	{
//		MessageBoxT(g_env.hMainWnd, "Could not apply theme", MB_ICONERROR);
	}

	Close(_resultOK);
}

void MapSettingsDlg::OnCancel()
{
	Close(_resultCancel);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
