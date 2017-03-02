#include "ConfigBinding.h"
#include "Settings.h"
#include "KeyMapper.h"
#include "inc/shell/Config.h"

#include <video/TextureManager.h>
#include <loc/Language.h>
#include <ui/Button.h>
#include <ui/Combo.h>
#include <ui/DataSourceAdapters.h>
#include <ui/Edit.h>
#include <ui/EditableText.h>
#include <ui/GuiManager.h>
#include <ui/Keys.h>
#include <ui/List.h>
#include <ui/ListBox.h>
#include <ui/ListSelectionBinding.h>
#include <ui/MultiColumnListItem.h>
#include <ui/Scroll.h>
#include <ui/StackLayout.h>
#include <ui/Text.h>

#include <algorithm>
#include <sstream>


SettingsDlg::SettingsDlg(UI::LayoutManager &manager, TextureManager &texman, ShellConfig &conf, LangCache &lang)
  : Managerful(manager)
  , _conf(conf)
  , _lang(lang)
{
	Resize(512, 296);

	auto text = std::make_shared<UI::Text>(texman);
	text->Move(GetWidth() / 2, 16);
	text->SetText(ConfBind(_lang.settings_title));
	text->SetAlign(alignTextCT);
	text->SetFont(texman, "font_default");
	AddFront(text);


	//
	// profiles
	//

	float x = 24;
	float y = 48;

	_content = std::make_shared<UI::StackLayout>();
	_content->SetSpacing(2);
	_content->Move(x, y);
	_content->Resize(128, 128);
	AddFront(_content);
	SetFocus(_content);

	text = std::make_shared<UI::Text>(texman);
	text->SetText(ConfBind(_lang.settings_player1));
	_content->AddFront(text);

	_player1 = std::make_shared<UI::ComboBox>(texman, &_profilesDataSource);
	_player1->GetList()->Resize(128, 52);
	_content->AddFront(_player1);

	text = std::make_shared<UI::Text>(texman);
	text->SetText(ConfBind(_lang.settings_player2));
	_content->AddFront(text);

	_player2 = std::make_shared<UI::ComboBox>(texman, &_profilesDataSource);
	_player2->GetList()->Resize(128, 52);
	_content->AddFront(_player2);

	text = std::make_shared<UI::Text>(texman);
	text->SetText(ConfBind(_lang.settings_profiles));
	_content->AddFront(text);

	_profiles = std::make_shared<UI::ListBox>(texman, &_profilesDataSource);
	_profiles->Resize(128, 52);
	_content->AddFront(_profiles);
	_content->SetFocus(_profiles);
	UpdateProfilesList(); // fill the list before binding OnChangeSel

	auto btn = std::make_shared<UI::Button>(texman);
	btn->SetText(ConfBind(_lang.settings_profile_new));
	btn->Move(40, 184);
	btn->eventClick = std::bind(&SettingsDlg::OnAddProfile, this);
	AddFront(btn);

	_editProfile = std::make_shared<UI::Button>(texman);
	_editProfile->SetText(ConfBind(_lang.settings_profile_edit));
	_editProfile->Move(40, 216);
	_editProfile->eventClick = std::bind(&SettingsDlg::OnEditProfile, this);
	_editProfile->SetEnabled(std::make_shared<UI::HasSelection>(_profiles->GetList()));
	AddFront(_editProfile);

	_deleteProfile = std::make_shared<UI::Button>(texman);
	_deleteProfile->SetText(ConfBind(_lang.settings_profile_delete));
	_deleteProfile->Move(40, 248);
	_deleteProfile->eventClick = std::bind(&SettingsDlg::OnDeleteProfile, this);
	_deleteProfile->SetEnabled(std::make_shared<UI::HasSelection>(_profiles->GetList()));
	AddFront(_deleteProfile);


	//
	// other settings
	//

	x = 200;
	y = 48;

	_content2 = std::make_shared<UI::StackLayout>();
	_content2->SetSpacing(2);
	_content2->Move(x, y);
	_content2->Resize(128, 128);
	AddFront(_content2);
	SetFocus(_content2);

	_showFps = std::make_shared<UI::CheckBox>(texman);
	_showFps->SetText(_lang.settings_show_fps.Get());
	_showFps->SetCheck(_conf.ui_showfps.Get());
	_showFps->eventClick = [=]
	{
		_conf.ui_showfps.Set(_showFps->GetCheck());
	};
	_content2->AddFront(_showFps);

	_showTime = std::make_shared<UI::CheckBox>(texman);
	_showTime->SetText(_lang.settings_show_time.Get());
	_showTime->SetCheck(_conf.ui_showtime.Get());
	_showTime->eventClick = [=]
	{
		_conf.ui_showtime.Set(_showTime->GetCheck());
	};
	_content2->AddFront(_showTime);

	_showNames = std::make_shared<UI::CheckBox>(texman);
	_showNames->SetText(_lang.settings_show_names.Get());
	_showNames->SetCheck(_conf.g_shownames.Get());
	_showNames->eventClick = [=]
	{
		_conf.g_shownames.Set(_showNames->GetCheck());
	};
	_content2->AddFront(_showNames);

	y += 100;

	text = std::make_shared<UI::Text>(texman);
	text->Move(x + 50, y += 20);
	text->SetText(ConfBind(_lang.settings_sfx_volume));
	text->SetAlign(alignTextRT);
	AddFront(text);

	_volumeSfx = std::make_shared<UI::ScrollBarHorizontal>(texman);
	_volumeSfx->Move(x + 60, y);
	_volumeSfx->SetWidth(150);
	_volumeSfx->SetDocumentSize(1);
	_volumeSfx->SetLineSize(0.1f);
	_volumeSfx->SetPos(expf(_conf.s_volume.GetFloat() / 2171.0f) - 0.01f);
	_volumeSfx->eventScroll = std::bind(&SettingsDlg::OnVolumeSfx, this, std::placeholders::_1);
	AddFront(_volumeSfx);
	_initialVolumeSfx = _conf.s_volume.GetInt();

	text = std::make_shared<UI::Text>(texman);
	text->Move(x + 50, y += 20);
	text->SetText(ConfBind(_lang.settings_music_volume));
	text->SetAlign(alignTextRT);
	AddFront(text);

	_volumeMusic = std::make_shared<UI::ScrollBarHorizontal>(texman);
	_volumeMusic->Move(x + 60, y);
	_volumeMusic->SetWidth(150);
	_volumeMusic->SetDocumentSize(1);
	_volumeMusic->SetLineSize(0.1f);
	_volumeMusic->SetPos(expf(_conf.s_musicvolume.GetFloat() / 2171.0f) - 0.01f);
	_volumeMusic->eventScroll = std::bind(&SettingsDlg::OnVolumeMusic, this, std::placeholders::_1);
	AddFront(_volumeMusic);
	_initialVolumeMusic = _conf.s_musicvolume.GetInt();

	_profiles->GetList()->SetCurSel(0, true);
	
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
	SetFocus(dlg);
}

void SettingsDlg::OnEditProfile()
{
	int i = _profiles->GetList()->GetCurSel();
	assert(i >= 0);
	auto dlg = std::make_shared<ControlProfileDlg>(GetManager(), GetManager().GetTextureManager(), _profilesDataSource.GetItemText(i, 0).c_str(), _conf, _lang);
	dlg->eventClose = std::bind(&SettingsDlg::OnProfileEditorClosed, this, std::placeholders::_1, std::placeholders::_2);
	AddFront(dlg);
	SetFocus(dlg);
}

void SettingsDlg::OnDeleteProfile()
{
	int i = _profiles->GetList()->GetCurSel();
	assert(i >= 0);
	if( _conf.cl_playerinfo.profile.Get() == _profilesDataSource.GetItemText(i, 0) )
	{
		// profile that is being deleted is used in network settings
		_conf.cl_playerinfo.profile.Set("");
	}
	_conf.dm_profiles.Remove(_profilesDataSource.GetItemText(i, 0));
	UpdateProfilesList();
}

void SettingsDlg::UpdateProfilesList()
{
	int sel = _profiles->GetList()->GetCurSel();
	std::vector<std::string> profiles = _conf.dm_profiles.GetKeys();
	_profilesDataSource.DeleteAllItems();
	for( size_t i = 0; i < profiles.size(); ++i )
	{
		_profilesDataSource.AddItem(profiles[i]);
	}
	_profiles->GetList()->SetCurSel(std::min(_profilesDataSource.GetItemCount() - 1, sel));
}

void SettingsDlg::OnProfileEditorClosed(std::shared_ptr<UI::Dialog> sender, int result)
{
	if( _resultOK == result )
	{
		UpdateProfilesList();
		SetFocus(_content);
	}
	UnlinkChild(*sender);
}

///////////////////////////////////////////////////////////////////////////////
// class ControlProfileDlg

static std::string GenerateProfileName(const ShellConfig &conf, LangCache &lang)
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

ControlProfileDlg::ControlProfileDlg(UI::LayoutManager &manager, TextureManager &texman, const char *profileName, ShellConfig &conf, LangCache &lang)
  : UI::Managerful(manager)
  , _nameOrig(profileName ? profileName : GenerateProfileName(conf, lang))
  , _profile(&conf.dm_profiles.GetTable(_nameOrig))
  , _conf(conf)
  , _lang(lang)
  , _activeIndex(-1)
  , _createNewProfile(!profileName)
{
	Resize(448, 416);

	auto text = std::make_shared<UI::Text>(texman);
	text->Move(20, 15);
	text->SetText(ConfBind(_lang.profile_name));
	AddFront(text);

	_nameEdit = std::make_shared<UI::Edit>(texman);
	_nameEdit->Move(20, 30);
	_nameEdit->SetWidth(250);
	_nameEdit->GetEditable()->SetText(_nameOrig);
	AddFront(_nameEdit);

	text = std::make_shared<UI::Text>(texman);
	text->Move(20, 65);
	text->SetText(ConfBind(_lang.profile_action));
	AddFront(text);

	text = std::make_shared<UI::Text>(texman);
	text->Move(220, 65);
	text->SetText(ConfBind(_lang.profile_key));
	AddFront(text);

	auto itemTemplate = std::make_shared<UI::MultiColumnListItem>(texman);
	itemTemplate->EnsureColumn(texman, 0, 2);
	itemTemplate->EnsureColumn(texman, 1, 200);

	_actions = std::make_shared<DefaultListBox>(texman);
	_actions->Move(20, 80);
	_actions->Resize(400, 250);
	_actions->GetList()->SetItemTemplate(itemTemplate);
	_actions->GetList()->eventClickItem = std::bind(&ControlProfileDlg::OnSelectAction, this, std::placeholders::_1);
	AddFront(_actions);

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
	_actions->GetList()->SetCurSel(0, true);

	_aimToMouseChkBox = std::make_shared<UI::CheckBox>(texman);
	_aimToMouseChkBox->SetCheck(_profile.aim_to_mouse.Get());
	_aimToMouseChkBox->Move(16, 345);
	_aimToMouseChkBox->SetText(_lang.profile_mouse_aim.Get());
	AddFront(_aimToMouseChkBox);

	_moveToMouseChkBox = std::make_shared<UI::CheckBox>(texman);
	_moveToMouseChkBox->SetCheck(_profile.move_to_mouse.Get());
	_moveToMouseChkBox->Move(146, 345);
	_moveToMouseChkBox->SetText(_lang.profile_mouse_move.Get());
	AddFront(_moveToMouseChkBox);

	_arcadeStyleChkBox = std::make_shared<UI::CheckBox>(texman);
	_arcadeStyleChkBox->SetCheck(_profile.arcade_style.Get());
	_arcadeStyleChkBox->Move(276, 345);
	_arcadeStyleChkBox->SetText(_lang.profile_arcade_style.Get());
	AddFront(_arcadeStyleChkBox);

	auto btn = std::make_shared<UI::Button>(texman);
	btn->SetText(ConfBind(_lang.common_ok));
	btn->Move(240, 380);
	btn->eventClick = std::bind(&ControlProfileDlg::OnOK, this);
	AddFront(btn);

	btn = std::make_shared<UI::Button>(texman);
	btn->SetText(ConfBind(_lang.common_cancel));
	btn->Move(344, 380);
	btn->eventClick = std::bind(&ControlProfileDlg::OnCancel, this);
	AddFront(btn);

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
	if( _nameEdit->GetEditable()->GetText().empty() || !_conf.dm_profiles.Rename(_profile, _nameEdit->GetEditable()->GetText()) )
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
	_actions->GetData()->SetItemText(_activeIndex, 1, fmodf(manager.GetTime(), 0.6f) > 0.3f ? "" : "...");
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
			if( GetFocus() == _actions && -1 != _actions->GetList()->GetCurSel() )
			{
				OnSelectAction(_actions->GetList()->GetCurSel());
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
