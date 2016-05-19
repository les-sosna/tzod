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
	class List;
	class ScrollBarHorizontal;
	template<class, class> class ListAdapter;
	enum class Key;
}

class SettingsDlg : public UI::Dialog
{
	UI::ListDataSourceDefault _profilesDataSource;

	std::shared_ptr<UI::ComboBox> _player1;
	std::shared_ptr<UI::ComboBox> _player2;
	std::shared_ptr<UI::List> _profiles;
	std::shared_ptr<UI::Button> _editProfile;
	std::shared_ptr<UI::Button> _deleteProfile;

	std::shared_ptr<UI::CheckBox> _showFps;
	std::shared_ptr<UI::CheckBox> _showTime;
	std::shared_ptr<UI::CheckBox> _showNames;
	std::shared_ptr<UI::CheckBox> _askDisplaySettings;

	std::shared_ptr<UI::ScrollBarHorizontal> _volumeSfx;
	int _initialVolumeSfx;

	std::shared_ptr<UI::ScrollBarHorizontal> _volumeMusic;
	int _initialVolumeMusic;

	ConfCache &_conf;
	LangCache &_lang;

public:
	SettingsDlg(UI::LayoutManager &manager, TextureManager &texman, ConfCache &conf, LangCache &lang);
	virtual ~SettingsDlg();

protected:
	void OnVolumeSfx(float pos);
	void OnVolumeMusic(float pos);

	void OnAddProfile();
	void OnEditProfile();
	void OnDeleteProfile();
	void OnSelectProfile(int index);

	void OnOK();
	void OnCancel();

	void UpdateProfilesList();
	void OnProfileEditorClosed(UI::Dialog &sender, int code);
};

class ControlProfileDlg : public UI::Dialog
{
public:
	ControlProfileDlg(UI::LayoutManager &manager, TextureManager &texman, const char *profileName, ConfCache &conf, LangCache &lang);
	~ControlProfileDlg();

	// UI::Window
	bool GetNeedsFocus() override;
	bool OnKeyPressed(UI::Key key) override;
	void OnTimeStep(float dt) override;

private:
	void AddAction(ConfVarString &var, std::string actionDisplayName);

	void OnOK();
	void OnCancel();

	void OnSelectAction(int index);
	void SetActiveIndex(int index);

	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	std::shared_ptr<DefaultListBox> _actions;
	std::shared_ptr<UI::Edit> _nameEdit;
	std::shared_ptr<UI::CheckBox> _aimToMouseChkBox;
	std::shared_ptr<UI::CheckBox> _moveToMouseChkBox;
	std::shared_ptr<UI::CheckBox> _arcadeStyleChkBox;
	std::string _nameOrig;
	std::vector<UI::Key> _keyBindings;
	ConfControllerProfile _profile;
	ConfCache &_conf;
	LangCache &_lang;

	float _time;
	int   _activeIndex;
	bool  _createNewProfile;
};
