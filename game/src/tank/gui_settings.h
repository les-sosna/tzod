#pragma once
#include "Config.h"
#include <ui/Dialog.h>
#include <ui/ListBase.h>

namespace UI
{
	class Button;
	class CheckBox;
	class ComboBox;
	class Edit;
	class List;
	class ScrollBarHorizontal;
	template<class, class> class ListAdapter;
}

class SettingsDlg : public UI::Dialog
{
	UI::ListDataSourceDefault _profilesDataSource;

	UI::ComboBox  *_player1;
	UI::ComboBox  *_player2;
	UI::List      *_profiles;
	UI::Button    *_editProfile;
	UI::Button    *_deleteProfile;

	UI::CheckBox  *_showFps;
	UI::CheckBox  *_showTime;
	UI::CheckBox  *_showNames;
	UI::CheckBox  *_askDisplaySettings;

	UI::ScrollBarHorizontal *_volumeSfx;
	int _initialVolumeSfx;

	UI::ScrollBarHorizontal *_volumeMusic;
	int _initialVolumeMusic;

public:
	SettingsDlg(UI::Window *parent);
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
	void OnProfileEditorClosed(int code);
};

class ControlProfileDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	DefaultListBox  *_actions;
	UI::Edit         *_nameEdit;
	UI::CheckBox     *_aimToMouseChkBox;
	UI::CheckBox     *_moveToMouseChkBox;
	UI::CheckBox     *_arcadeStyleChkBox;
	std::string      _nameOrig;
	ConfControllerProfile _profile;

	float _time;
	int   _activeIndex;
	bool  _createNewProfile;

	void OnSelectAction(int index);

public:
	ControlProfileDlg(UI::Window *parent, const char *profileName);
	~ControlProfileDlg();

protected:
	void AddAction(ConfVarString &var, const std::string &display);

	void OnOK();
	void OnCancel();

	void OnTimeStep(float dt);
	bool OnRawChar(int c);
};
