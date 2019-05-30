#include "Settings.h"
#include "KeyMapper.h"
#include "inc/shell/Config.h"

#include <cbind/ConfigBinding.h>
#include <video/TextureManager.h>
#include <loc/Language.h>
#include <plat/Keys.h>
#include <ui/Button.h>
#include <ui/Combo.h>
#include <ui/DataSourceAdapters.h>
#include <ui/Edit.h>
#include <ui/EditableText.h>
#include <ui/List.h>
#include <ui/ListBox.h>
#include <ui/ListSelectionBinding.h>
#include <ui/MultiColumnListItem.h>
#include <ui/ScrollBar.h>
#include <ui/StackLayout.h>
#include <ui/Text.h>

#include <algorithm>
#include <sstream>


SettingsDlg::SettingsDlg(TextureManager &texman, ShellConfig &conf, LangCache &lang)
  : _conf(conf)
  , _lang(lang)
{
	Resize(512, 296);

	auto title = std::make_shared<UI::Text>();
	title->Move(GetWidth() / 2, 16);
	title->SetText(ConfBind(_lang.settings_title));
	title->SetAlign(alignTextCT);
	title->SetFont("font_default");
	AddFront(title);


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
	SetFocus(_content.get());

	auto text = std::make_shared<UI::Text>();
	text->SetText(ConfBind(_lang.settings_player1));
	_content->AddFront(text);

	_player1 = std::make_shared<UI::ComboBox>(&_profilesDataSource);
	_player1->GetList()->Resize(128, 52);
	_content->AddFront(_player1);

	text = std::make_shared<UI::Text>();
	text->SetText(ConfBind(_lang.settings_player2));
	_content->AddFront(text);

	_player2 = std::make_shared<UI::ComboBox>(&_profilesDataSource);
	_player2->GetList()->Resize(128, 52);
	_content->AddFront(_player2);

	text = std::make_shared<UI::Text>();
	text->SetText(ConfBind(_lang.settings_profiles));
	_content->AddFront(text);

	_profiles = std::make_shared<UI::ListBox>(&_profilesDataSource);
	_profiles->Resize(128, 52);
	_content->AddFront(_profiles);
	_content->SetFocus(_profiles.get());
	UpdateProfilesList(); // fill the list before binding OnChangeSel

	auto btn = std::make_shared<UI::Button>();
	btn->SetText(ConfBind(_lang.settings_profile_new));
	btn->Move(40, 184);
	btn->eventClick = std::bind(&SettingsDlg::OnAddProfile, this);
	AddFront(btn);

	_editProfile = std::make_shared<UI::Button>();
	_editProfile->SetText(ConfBind(_lang.settings_profile_edit));
	_editProfile->Move(40, 216);
	_editProfile->eventClick = std::bind(&SettingsDlg::OnEditProfile, this);
//	_editProfile->SetEnabled(std::make_shared<UI::HasSelection>(_profiles->GetList()));
	AddFront(_editProfile);

	_deleteProfile = std::make_shared<UI::Button>();
	_deleteProfile->SetText(ConfBind(_lang.settings_profile_delete));
	_deleteProfile->Move(40, 248);
	_deleteProfile->eventClick = std::bind(&SettingsDlg::OnDeleteProfile, this);
//	_deleteProfile->SetEnabled(std::make_shared<UI::HasSelection>(_profiles->GetList()));
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
	SetFocus(_content2.get());

	_showFps = std::make_shared<UI::CheckBox>();
	_showFps->SetText(ConfBind(_lang.settings_show_fps));
	_showFps->SetCheck(_conf.d_showfps.Get());
	_showFps->eventClick = [=]
	{
		_conf.d_showfps.Set(_showFps->GetCheck());
	};
	_content2->AddFront(_showFps);

	_showNames = std::make_shared<UI::CheckBox>();
	_showNames->SetText(ConfBind(_lang.settings_show_names));
	_showNames->SetCheck(_conf.g_shownames.Get());
	_showNames->eventClick = [=]
	{
		_conf.g_shownames.Set(_showNames->GetCheck());
	};
	_content2->AddFront(_showNames);

	y += 100;

	text = std::make_shared<UI::Text>();
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

	text = std::make_shared<UI::Text>();
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

	_profiles->GetList()->SetCurSel(0);
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
	auto dlg = std::make_shared<ControlProfileDlg>(std::string_view(), _conf, _lang);
	dlg->eventClose = [this, weakSender = std::weak_ptr<ControlProfileDlg>(dlg)](int result)
	{
		OnProfileEditorClosed(weakSender.lock(), result);
	};
	AddFront(dlg);
	SetFocus(dlg.get());
}

void SettingsDlg::OnEditProfile()
{
	int i = _profiles->GetList()->GetCurSel();
	assert(i >= 0);
	auto dlg = std::make_shared<ControlProfileDlg>(_profilesDataSource.GetItemText(i, 0), _conf, _lang);
	dlg->eventClose = [this, weakSender = std::weak_ptr<ControlProfileDlg>(dlg)](int result)
	{
		OnProfileEditorClosed(weakSender.lock(), result);
	};
	AddFront(dlg);
	SetFocus(dlg.get());
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
		SetFocus(_content.get());
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

ControlProfileDlg::ControlProfileDlg(std::string_view profileName, ShellConfig &conf, LangCache &lang)
  : _nameOrig(profileName.empty() ? GenerateProfileName(conf, lang) : profileName)
  , _profile(&conf.dm_profiles.GetTable(_nameOrig))
  , _conf(conf)
  , _lang(lang)
  , _activeIndex(-1)
  , _createNewProfile(profileName.empty())
{
	Resize(448, 416);

	auto text = std::make_shared<UI::Text>();
	text->Move(20, 15);
	text->SetText(ConfBind(_lang.profile_name));
	AddFront(text);

	_nameEdit = std::make_shared<UI::Edit>();
	_nameEdit->Move(20, 30);
	_nameEdit->SetWidth(250);
	_nameEdit->GetEditable().SetText(_nameOrig);
	AddFront(_nameEdit);

	text = std::make_shared<UI::Text>();
	text->Move(20, 65);
	text->SetText(ConfBind(_lang.profile_action));
	AddFront(text);

	text = std::make_shared<UI::Text>();
	text->Move(220, 65);
	text->SetText(ConfBind(_lang.profile_key));
	AddFront(text);

	auto itemTemplate = std::make_shared<UI::MultiColumnListItem>();
	itemTemplate->EnsureColumn(0, 2);
	itemTemplate->EnsureColumn(1, 200);

	_actions = std::make_shared<DefaultListBox>();
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
	AddAction(_profile.key_pickup       , _lang.action_pickup.Get()        );
	_actions->GetList()->SetCurSel(0);

	_aimToMouseChkBox = std::make_shared<UI::CheckBox>();
	_aimToMouseChkBox->SetCheck(_profile.aim_to_mouse.Get());
	_aimToMouseChkBox->Move(16, 345);
	_aimToMouseChkBox->Resize(100, 20);
	_aimToMouseChkBox->SetText(ConfBind(_lang.profile_mouse_aim));
	AddFront(_aimToMouseChkBox);

	_moveToMouseChkBox = std::make_shared<UI::CheckBox>();
	_moveToMouseChkBox->SetCheck(_profile.move_to_mouse.Get());
	_moveToMouseChkBox->Move(146, 345);
	_moveToMouseChkBox->Resize(100, 20);
	_moveToMouseChkBox->SetText(ConfBind(_lang.profile_mouse_move));
	AddFront(_moveToMouseChkBox);

	_arcadeStyleChkBox = std::make_shared<UI::CheckBox>();
	_arcadeStyleChkBox->SetCheck(_profile.arcade_style.Get());
	_arcadeStyleChkBox->Move(276, 345);
	_arcadeStyleChkBox->Resize(100, 20);
	_arcadeStyleChkBox->SetText(ConfBind(_lang.profile_arcade_style));
	AddFront(_arcadeStyleChkBox);

	auto btn = std::make_shared<UI::Button>();
	btn->SetText(ConfBind(_lang.common_ok));
	btn->Move(240, 380);
	btn->eventClick = std::bind(&ControlProfileDlg::OnOK, this);
	AddFront(btn);

	btn = std::make_shared<UI::Button>();
	btn->SetText(ConfBind(_lang.common_cancel));
	btn->Move(344, 380);
	btn->eventClick = std::bind(&ControlProfileDlg::OnCancel, this);
	AddFront(btn);

	SetFocus(_actions.get());
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
		_actions->GetData()->SetItemText(_activeIndex, 1, "Press a key...");
	}
}

void ControlProfileDlg::AddAction(ConfVarString &keyName, std::string_view actionDisplayName)
{
	_keyBindings.push_back(GetKeyCode(keyName.Get()));
	int index = _actions->GetData()->AddItem(std::move(actionDisplayName), reinterpret_cast<size_t>(&keyName));
	_actions->GetData()->SetItemText(index, 1, GetKeyName(_keyBindings.back()));
	assert(_actions->GetData()->GetItemCount() == _keyBindings.size());
}

void ControlProfileDlg::OnOK()
{
	if( _nameEdit->GetEditable().GetText().empty() || !_conf.dm_profiles.Rename(_profile, std::string(_nameEdit->GetEditable().GetText())) )
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

bool ControlProfileDlg::OnKeyPressed(const UI::InputContext &ic, Plat::Key key)
{
	if (-1 != _activeIndex)
	{
		if (Plat::Key::Escape != key)
		{
			_keyBindings[_activeIndex] = key;
		}
		SetActiveIndex(-1);
		SetFocus(_actions.get());
	}
	else
	{
		switch( key )
		{
		case Plat::Key::Enter:
			if( GetFocus() == _actions.get() && -1 != _actions->GetList()->GetCurSel() )
			{
				OnSelectAction(_actions->GetList()->GetCurSel());
			}
			else
			{
				OnOK();
			}
			break;
		case Plat::Key::Escape:
			break;
		default:
			return Dialog::OnKeyPressed(ic, key);
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

#include "ConfigWidgets.h"

static const float c_buttonWidth = 200;
static const float c_buttonHeight = 50;

MainSettingsDlg::MainSettingsDlg(LangCache& lang, MainSettingsCommands commands)
{
	SetFlowDirection(UI::FlowDirection::Vertical);
	SetSpacing(20);

	std::shared_ptr<UI::Button> button;

	button = std::make_shared<UI::Button>();
	button->SetFont("font_default");
	button->SetText(ConfBind(lang.settings_player));
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = commands.player;
	AddFront(button);
	SetFocus(button.get());

	button = std::make_shared<UI::Button>();
	button->SetFont("font_default");
	button->SetText(ConfBind(lang.settings_controls));
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = commands.controls;
	AddFront(button);

	button = std::make_shared<UI::Button>();
	button->SetFont("font_default");
	button->SetText(ConfBind(lang.settings_advanced));
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = commands.advanced;
	AddFront(button);
}

///////////////////////////////////////////////////////////////////////////////

SettingsListBase::SettingsListBase()
{
	SetFlowDirection(UI::FlowDirection::Vertical);
	SetSpacing(20);
}

vec2d SettingsListBase::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	auto stackContentSize = UI::StackLayout::GetContentSize(texman, dc, scale, layoutConstraints);
	return vec2d{ std::floor(400 * scale), stackContentSize.y };
}

template <class WidgetType, class ConfVarType>
void SettingsListBase::AddSetting(const ConfVarString& title, ConfVarType& confVar)
{
	auto widget = std::make_shared<WidgetType>(confVar);
	widget->SetTitle(ConfBind(title));
	AddFront(widget);
	if (!GetFocus())
		SetFocus(widget.get());
}

///////////////////////////////////////////////////////////////////////////////

PlayerSettings::PlayerSettings(ShellConfig& conf, LangCache& lang)
{
	AddSetting<StringSetting>(lang.settings_player_nick, conf.cl_playerinfo.nick);
}

ControlsSettings::ControlsSettings(ShellConfig& conf, LangCache& lang)
{
	ConfControllerProfile profile(&conf.dm_profiles.GetTable(conf.dm_profiles.GetKeys().front()));

	AddSetting<BooleanSetting>(lang.profile_mouse_aim, profile.aim_to_mouse);
	AddSetting<BooleanSetting>(lang.profile_mouse_move, profile.move_to_mouse);
	AddSetting<BooleanSetting>(lang.profile_arcade_style, profile.arcade_style);

	AddSetting<KeyBindSetting>(lang.action_move_forward, profile.key_forward);
	AddSetting<KeyBindSetting>(lang.action_move_backward, profile.key_back);
	AddSetting<KeyBindSetting>(lang.action_turn_left, profile.key_left);
	AddSetting<KeyBindSetting>(lang.action_turn_right, profile.key_right);
	AddSetting<KeyBindSetting>(lang.action_fire, profile.key_fire);
	AddSetting<KeyBindSetting>(lang.action_toggle_lights, profile.key_light);
	AddSetting<KeyBindSetting>(lang.action_tower_left, profile.key_tower_left);
	AddSetting<KeyBindSetting>(lang.action_tower_center, profile.key_tower_center);
	AddSetting<KeyBindSetting>(lang.action_tower_right, profile.key_tower_right);
	AddSetting<KeyBindSetting>(lang.action_pickup, profile.key_pickup);
}

AdvancedSettings::AdvancedSettings(ShellConfig& conf, LangCache& lang)
{
	AddSetting<BooleanSetting>(lang.settings_advanced_fullscreen, conf.r_fullscreen);
	AddSetting<BooleanSetting>(lang.settings_advanced_vsync, conf.r_vsync);
	AddSetting<BooleanSetting>(lang.settings_advanced_framestats, conf.d_showfps);
}
