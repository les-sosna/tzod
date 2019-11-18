#pragma once
#include "inc/shell/Config.h"
#include <ui/Dialog.h>
#include <ui/ListBase.h>
#include <ui/ScrollView.h>
#include <string>
#include <vector>

class LangCache;
class TextureManager;
namespace UI
{
	class Button;
	class CheckBox;
	class ComboBox;
	class Edit;
	class ListBox;
	class ScrollBarHorizontal;
	class StackLayout;
	template<class, class> class ListAdapter;
	enum class Key;
}

class SettingsDlg : public UI::Dialog
{
public:
	SettingsDlg(TextureManager &texman, ShellConfig &conf, LangCache &lang);
	virtual ~SettingsDlg();

protected:
	void OnVolumeSfx(float pos);
	void OnVolumeMusic(float pos);

	void OnAddProfile();
	void OnEditProfile();
	void OnDeleteProfile();

	void UpdateProfilesList();
	void OnProfileEditorClosed(std::shared_ptr<UI::Dialog> sender, int result);

private:
	UI::ListDataSourceDefault _profilesDataSource;

	std::shared_ptr<UI::StackLayout> _content;
	std::shared_ptr<UI::StackLayout> _content2;

	std::shared_ptr<UI::ComboBox> _player1;
	std::shared_ptr<UI::ComboBox> _player2;
	std::shared_ptr<UI::ListBox> _profiles;
	std::shared_ptr<UI::Button> _editProfile;
	std::shared_ptr<UI::Button> _deleteProfile;

	std::shared_ptr<UI::CheckBox> _showFps;
	std::shared_ptr<UI::CheckBox> _showNames;

	std::shared_ptr<UI::ScrollBarHorizontal> _volumeSfx;
	int _initialVolumeSfx;

	std::shared_ptr<UI::ScrollBarHorizontal> _volumeMusic;
	int _initialVolumeMusic;

	ShellConfig &_conf;
	LangCache &_lang;
};

class ControlProfileDlg : public UI::Dialog
{
public:
	ControlProfileDlg(std::string_view profileName, ShellConfig &conf, LangCache &lang);
	~ControlProfileDlg();

	// UI::KeyboardSink
	bool OnKeyPressed(const Plat::Input &input, const UI::InputContext &ic, Plat::Key key) override;

private:
	void AddAction(ConfVarString &var, std::string_view actionDisplayName);

	void OnOK();
	void OnCancel();

	void OnSelectAction(int index);
	void SetActiveIndex(int index);

	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ListBox> DefaultListBox;
	std::shared_ptr<DefaultListBox> _actions;
	std::shared_ptr<UI::Edit> _nameEdit;
	std::shared_ptr<UI::CheckBox> _aimToMouseChkBox;
	std::shared_ptr<UI::CheckBox> _moveToMouseChkBox;
	std::shared_ptr<UI::CheckBox> _arcadeStyleChkBox;
	std::string _nameOrig;
	std::vector<Plat::Key> _keyBindings;
	ConfControllerProfile _profile;
	ShellConfig &_conf;
	LangCache &_lang;

	int   _activeIndex;
	bool  _createNewProfile;
};


#include <ui/StackLayout.h>

struct MainSettingsCommands
{
	std::function<void()> player;
	std::function<void()> controls;
	std::function<void()> advanced;
};

class MainSettingsDlg : public UI::StackLayout
{
public:
	MainSettingsDlg(LangCache& lang, MainSettingsCommands commands);
};

class SettingsListBase : public UI::Window
{
    TUPLE_CHILDREN(UI::ScrollView);
    STATIC_FOCUS(0);

public:
	SettingsListBase();

	// UI::Window
	vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;
    UI::WindowLayout GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override;

protected:
	template <class WidgetType, class ConfVarType>
	void AddSetting(const ConfVarString& title, ConfVarType& confVar);
};

class PlayerSettings : public SettingsListBase
{
public:
	PlayerSettings(AppConfig &conf, LangCache& lang);
};

class ControlsSettings : public SettingsListBase
{
public:
	ControlsSettings(ShellConfig& conf, LangCache& lang);
};

class AdvancedSettings : public SettingsListBase
{
public:
	AdvancedSettings(ShellConfig& conf, LangCache& lang);
};
