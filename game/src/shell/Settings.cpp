#include "Settings.h"
#include "KeyMapper.h"
#include "inc/shell/Config.h"

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
#include <ui/Keys.h>

#include <algorithm>
#include <sstream>


SettingsDlg::SettingsDlg(UI::LayoutManager &manager, TextureManager &texman, ConfCache &conf, LangCache &lang)
  : Dialog(manager, texman, 512, 296)
  , _conf(conf)
  , _lang(lang)
{
	SetEasyMove(true);

	auto title = UI::Text::Create(this, texman, GetWidth() / 2, 16, _lang.settings_title.Get(), alignTextCT);
	title->SetFont(texman, "font_default");


	//
	// profiles
	//

	float x = 24;
	float y = 48;

	y += UI::Text::Create(this, texman, x, y, _lang.settings_player1.Get(), alignTextLT)->GetHeight() + 2;
	_player1 = std::make_shared<UI::ComboBox>(manager, texman, &_profilesDataSource);
	_player1->Move(x, y);
	_player1->Resize(128);
	_player1->GetList()->Resize(128, 52);
	AddFront(_player1);
	y += _player1->GetHeight() + 5;

	y += UI::Text::Create(this, texman, 24, y, _lang.settings_player2.Get(), alignTextLT)->GetHeight() + 2;
	_player2 = std::make_shared<UI::ComboBox>(manager, texman, &_profilesDataSource);
	_player2->Move(x, y);
	_player2->Resize(128);
	_player2->GetList()->Resize(128, 52);
	AddFront(_player2);
	y += _player2->GetHeight() + 5;

	y += UI::Text::Create(this, texman, x, y, _lang.settings_profiles.Get(), alignTextLT)->GetHeight() + 2;
	_profiles = std::make_shared<UI::List>(manager, texman, &_profilesDataSource);
	_profiles->Move(x, y);
	_profiles->Resize(128, 52);
	AddFront(_profiles);
	UpdateProfilesList(); // fill the list before binding OnChangeSel
	_profiles->eventChangeCurSel = std::bind(&SettingsDlg::OnSelectProfile, this, std::placeholders::_1);

	UI::Button::Create(this, texman, _lang.settings_profile_new.Get(), 40, 184)->eventClick = std::bind(&SettingsDlg::OnAddProfile, this);
	_editProfile = UI::Button::Create(this, texman, _lang.settings_profile_edit.Get(), 40, 216);
	_editProfile->eventClick = std::bind(&SettingsDlg::OnEditProfile, this);
	_editProfile->SetEnabled(false);
	_deleteProfile = UI::Button::Create(this, texman, _lang.settings_profile_delete.Get(), 40, 248);
	_deleteProfile->eventClick = std::bind(&SettingsDlg::OnDeleteProfile, this);
	_deleteProfile->SetEnabled(false);


	//
	// other settings
	//

	x = 200;
	y = 48;

	_showFps = UI::CheckBox::Create(this, texman, x, y, _lang.settings_show_fps.Get());
	_showFps->SetCheck(_conf.ui_showfps.Get());
	y += _showFps->GetHeight();

	_showTime = UI::CheckBox::Create(this, texman, x, y, _lang.settings_show_time.Get());
	_showTime->SetCheck(_conf.ui_showtime.Get());
	y += _showTime->GetHeight();

	_showNames = UI::CheckBox::Create(this, texman, x, y, _lang.settings_show_names.Get());
	_showNames->SetCheck(_conf.g_shownames.Get());
	y += _showNames->GetHeight();

	_askDisplaySettings = UI::CheckBox::Create(this, texman, x, y, _lang.settings_ask_for_display_mode.Get());
	_askDisplaySettings->SetCheck(_conf.r_askformode.Get());
	y += _askDisplaySettings->GetHeight();

	UI::Text::Create(this, texman, x + 50, y += 20, _lang.settings_sfx_volume.Get(), alignTextRT);
	_volumeSfx = std::make_shared<UI::ScrollBarHorizontal>(manager, texman);
	_volumeSfx->Move(x + 60, y);
	_volumeSfx->SetSize(150);
	_volumeSfx->SetDocumentSize(1);
	_volumeSfx->SetLineSize(0.1f);
	_volumeSfx->SetPos(expf(_conf.s_volume.GetFloat() / 2171.0f) - 0.01f);
	_volumeSfx->eventScroll = std::bind(&SettingsDlg::OnVolumeSfx, this, std::placeholders::_1);
	AddFront(_volumeSfx);
	_initialVolumeSfx = _conf.s_volume.GetInt();

	UI::Text::Create(this, texman, x + 50, y += 20, _lang.settings_music_volume.Get(), alignTextRT);
	_volumeMusic = std::make_shared<UI::ScrollBarHorizontal>(manager, texman);
	_volumeMusic->Move(x + 60, y);
	_volumeMusic->SetSize(150);
	_volumeMusic->SetDocumentSize(1);
	_volumeMusic->SetLineSize(0.1f);
	_volumeMusic->SetPos(expf(_conf.s_musicvolume.GetFloat() / 2171.0f) - 0.01f);
	_volumeMusic->eventScroll = std::bind(&SettingsDlg::OnVolumeMusic, this, std::placeholders::_1);
	AddFront(_volumeMusic);
	_initialVolumeMusic = _conf.s_musicvolume.GetInt();


	//
	// OK & Cancel
	//

	UI::Button::Create(this, texman, _lang.common_ok.Get(), 304, 256)->eventClick = std::bind(&SettingsDlg::OnOK, this);
	UI::Button::Create(this, texman, _lang.common_cancel.Get(), 408, 256)->eventClick = std::bind(&SettingsDlg::OnCancel, this);

	_profiles->SetCurSel(0, true);
	SetFocus(_profiles);
}

SettingsDlg::~SettingsDlg()
{
}

void SettingsDlg::OnVolumeSfx(float pos)
{
	_conf.s_volume.SetInt( int(2171.0f * logf(0.01f + pos)) );
}

void SettingsDlg::OnVolumeMusic(float pos)
{
	_conf.s_musicvolume.SetInt( int(2171.0f * logf(0.01f + pos)) );
}

void SettingsDlg::OnAddProfile()
{
	auto dlg = std::make_shared<ControlProfileDlg>(GetManager(), GetManager().GetTextureManager(), nullptr, _conf, _lang);
	dlg->eventClose = std::bind(&SettingsDlg::OnProfileEditorClosed, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
}

void SettingsDlg::OnEditProfile()
{
	int i = _profiles->GetCurSel();
	assert(i >= 0);
	auto dlg = std::make_shared<ControlProfileDlg>(GetManager(), GetManager().GetTextureManager(), _profilesDataSource.GetItemText(i, 0).c_str(), _conf, _lang);
	dlg->eventClose = std::bind(&SettingsDlg::OnProfileEditorClosed, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
}

void SettingsDlg::OnDeleteProfile()
{
	int i = _profiles->GetCurSel();
	assert(i >= 0);
	if( _conf.cl_playerinfo.profile.Get() == _profilesDataSource.GetItemText(i, 0) )
	{
		// profile that is being deleted is used in network settings
		_conf.cl_playerinfo.profile.Set("");
	}
	_conf.dm_profiles.Remove(_profilesDataSource.GetItemText(i, 0));
	UpdateProfilesList();
}

void SettingsDlg::OnSelectProfile(int index)
{
	_editProfile->SetEnabled( -1 != index );
	_deleteProfile->SetEnabled( -1 != index );
}

void SettingsDlg::OnOK()
{
	_conf.ui_showfps.Set(_showFps->GetCheck());
	_conf.ui_showtime.Set(_showTime->GetCheck());
	_conf.g_shownames.Set(_showNames->GetCheck());
	_conf.r_askformode.Set(_askDisplaySettings->GetCheck());

	Close(_resultCancel); // return cancel to show back the main menu
}

void SettingsDlg::OnCancel()
{
	_conf.s_volume.SetInt(_initialVolumeSfx);
	_conf.s_musicvolume.SetInt(_initialVolumeMusic);
	Close(_resultCancel);
}

void SettingsDlg::UpdateProfilesList()
{
	int sel = _profiles->GetCurSel();
	std::vector<std::string> profiles = _conf.dm_profiles.GetKeys();
	_profilesDataSource.DeleteAllItems();
	for( size_t i = 0; i < profiles.size(); ++i )
	{
		_profilesDataSource.AddItem(profiles[i]);
	}
	_profiles->SetCurSel(std::min(_profilesDataSource.GetItemCount() - 1, sel));
}

void SettingsDlg::OnProfileEditorClosed(std::shared_ptr<UI::Dialog> sender, int result)
{
	if( _resultOK == result )
	{
		UpdateProfilesList();
		SetFocus(_profiles);
	}
}

///////////////////////////////////////////////////////////////////////////////
// class ControlProfileDlg

static std::string GenerateProfileName(const ConfCache &conf, LangCache &lang)
{
	int i = 0;
	std::ostringstream buf;
	do
	{
		buf.str("");
		buf << lang.profile_autoname.Get() << ++i;
	}
	while( conf.dm_profiles.Find(buf.str()) );
	return buf.str();
}

ControlProfileDlg::ControlProfileDlg(UI::LayoutManager &manager, TextureManager &texman, const char *profileName, ConfCache &conf, LangCache &lang)
  : Dialog(manager, texman, 448, 416)
  , _nameOrig(profileName ? profileName : GenerateProfileName(conf, lang))
  , _profile(&conf.dm_profiles.GetTable(_nameOrig))
  , _conf(conf)
  , _lang(lang)
  , _time(0)
  , _activeIndex(-1)
  , _createNewProfile(!profileName)
{
	SetEasyMove(true);

	UI::Text::Create(this, texman, 20, 15, _lang.profile_name.Get(), alignTextLT);
	_nameEdit = std::make_shared<UI::Edit>(manager, texman);
	_nameEdit->Move(20, 30);
	_nameEdit->SetWidth(250);
	_nameEdit->SetText(_nameOrig);
	AddFront(_nameEdit);

	UI::Text::Create(this, texman,  20, 65, _lang.profile_action.Get(), alignTextLT);
	UI::Text::Create(this, texman, 220, 65, _lang.profile_key.Get(), alignTextLT);
	_actions = DefaultListBox::Create(this, texman);
	_actions->Move(20, 80);
	_actions->Resize(400, 250);
	_actions->SetTabPos(0, 2);
	_actions->SetTabPos(1, 200);
	_actions->eventClickItem = std::bind(&ControlProfileDlg::OnSelectAction, this, std::placeholders::_1);

	AddAction(_profile.key_forward      , _lang.action_move_forward.Get()  );
	AddAction(_profile.key_back         , _lang.action_move_backward.Get() );
	AddAction(_profile.key_left         , _lang.action_turn_left.Get()     );
	AddAction(_profile.key_right        , _lang.action_turn_right.Get()    );
	AddAction(_profile.key_fire         , _lang.action_fire.Get()          );
	AddAction(_profile.key_light        , _lang.action_toggle_lights.Get() );
	AddAction(_profile.key_tower_left   , _lang.action_tower_left.Get()    );
	AddAction(_profile.key_tower_right  , _lang.action_tower_right.Get()   );
	AddAction(_profile.key_tower_center , _lang.action_tower_center.Get()  );
	AddAction(_profile.key_no_pickup    , _lang.action_no_pickup.Get()     );
	_actions->SetCurSel(0, true);

	_aimToMouseChkBox = UI::CheckBox::Create(this, texman, 16, 345, _lang.profile_mouse_aim.Get());
	_aimToMouseChkBox->SetCheck(_profile.aim_to_mouse.Get());

	_moveToMouseChkBox = UI::CheckBox::Create(this, texman, 146, 345, _lang.profile_mouse_move.Get());
	_moveToMouseChkBox->SetCheck(_profile.move_to_mouse.Get());

	_arcadeStyleChkBox = UI::CheckBox::Create(this, texman, 276, 345, _lang.profile_arcade_style.Get());
	_arcadeStyleChkBox->SetCheck(_profile.arcade_style.Get());

	UI::Button::Create(this, texman, _lang.common_ok.Get(), 240, 380)->eventClick = std::bind(&ControlProfileDlg::OnOK, this);
	UI::Button::Create(this, texman, _lang.common_cancel.Get(), 344, 380)->eventClick = std::bind(&ControlProfileDlg::OnCancel, this);

	SetFocus(_actions);
}

ControlProfileDlg::~ControlProfileDlg()
{
}

void ControlProfileDlg::OnSelectAction(int index)
{
	SetActiveIndex(index);
	SetFocus(nullptr);
}

void ControlProfileDlg::SetActiveIndex(int index)
{
	if (index == _activeIndex)
		return;

	if (_activeIndex != -1)
	{
		_actions->GetData()->SetItemText(_activeIndex, 1, GetKeyName(_keyBindings[_activeIndex]));
	}

	_activeIndex = index;

	if (_activeIndex != -1)
	{
		_actions->GetData()->SetItemText(_activeIndex, 1, "...");
		SetTimeStep(true);
		_time = 0;
	}
	else
	{
		SetTimeStep(false);
	}
}

void ControlProfileDlg::AddAction(ConfVarString &keyName, std::string actionDisplayName)
{
	_keyBindings.push_back(GetKeyCode(keyName.Get()));
	int index = _actions->GetData()->AddItem(std::move(actionDisplayName), reinterpret_cast<size_t>(&keyName));
	_actions->GetData()->SetItemText(index, 1, GetKeyName(_keyBindings.back()));
	assert(_actions->GetData()->GetItemCount() == _keyBindings.size());
}

void ControlProfileDlg::OnOK()
{
	if( _nameEdit->GetText().empty() || !_conf.dm_profiles.Rename(_profile, _nameEdit->GetText()) )
	{
		return;
	}

	for( int i = 0; i < _actions->GetData()->GetItemCount(); ++i )
	{
		reinterpret_cast<ConfVarString*>(_actions->GetData()->GetItemData(i))->Set(GetKeyName(_keyBindings[i]));
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
		_conf.dm_profiles.Remove(_profile);
	}
	Close(_resultCancel);
}

void ControlProfileDlg::OnTimeStep(UI::LayoutManager &manager, float dt)
{
	_time += dt;
	_actions->GetData()->SetItemText(_activeIndex, 1, fmodf(_time, 0.6f) > 0.3f ? "" : "...");
}

bool ControlProfileDlg::OnKeyPressed(UI::InputContext &ic, UI::Key key)
{
	if (-1 != _activeIndex)
	{
		if (UI::Key::Escape != key)
		{
			_keyBindings[_activeIndex] = key;
		}
		SetActiveIndex(-1);
		SetFocus(_actions);
	}
	else
	{
		switch( key )
		{
		case UI::Key::Enter:
			if( GetFocus() == _actions && -1 != _actions->GetCurSel() )
			{
				OnSelectAction(_actions->GetCurSel());
			}
			else
			{
				OnOK();
			}
			break;
		case UI::Key::Escape:
			break;
		default:
			return Dialog::OnKeyPressed(ic, key);
		}
	}
	return true;
}
