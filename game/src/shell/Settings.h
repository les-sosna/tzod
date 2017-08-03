#pragma once
#include "inc/shell/Config.h"
#include <ui/Dialog.h>
#include <ui/ListBase.h>
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
	UI::ListDataSourceDefault _profilesDataSource;

	std::shared_ptr<UI::StackLayout> _content;
	std::shared_ptr<UI::StackLayout> _content2;

	std::shared_ptr<UI::ComboBox> _player1;
	std::shared_ptr<UI::ComboBox> _player2;
	std::shared_ptr<UI::ListBox> _profiles;
	std::shared_ptr<UI::Button> _editProfile;
	std::shared_ptr<UI::Button> _deleteProfile;

	std::shared_ptr<UI::CheckBox> _showFps;
	std::shared_ptr<UI::CheckBox> _showTime;
	std::shared_ptr<UI::CheckBox> _showNames;

	std::shared_ptr<UI::ScrollBarHorizontal> _volumeSfx;
	int _initialVolumeSfx;

	std::shared_ptr<UI::ScrollBarHorizontal> _volumeMusic;
	int _initialVolumeMusic;

	ShellConfig &_conf;
	LangCache &_lang;

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
};

class ControlProfileDlg : public UI::Dialog
{
public:
	ControlProfileDlg(std::string_view profileName, ShellConfig &conf, LangCache &lang);
	~ControlProfileDlg();

	// UI::Window
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;

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
	std::vector<UI::Key> _keyBindings;
	ConfControllerProfile _profile;
	ShellConfig &_conf;
	LangCache &_lang;

	int   _activeIndex;
	bool  _createNewProfile;
};
