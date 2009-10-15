// gui_settings.h

#pragma once

#include "ui/Base.h"
#include "ui/Dialog.h"
#include "config/Config.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class SettingsDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox *_profiles;
	Button    *_editProfile;
	Button    *_deleteProfile;

	CheckBox  *_particles;
	CheckBox  *_showFps;
	CheckBox  *_showTime;
	CheckBox  *_showDamage;
	CheckBox  *_showNames;
	CheckBox  *_askDisplaySettings;

	ScrollBarHorizontal *_volumeSfx;
	int _initialVolumeSfx;

	ScrollBarHorizontal *_volumeMusic;
	int _initialVolumeMusic;


public:
	SettingsDlg(Window *parent);
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

///////////////////////////////////////////////////////////////////////////////

class ControlProfileDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, List> DefaultListBox;
	DefaultListBox  *_actions;
	Edit         *_nameEdit;
	CheckBox     *_aimToMouseChkBox;
	CheckBox     *_moveToMouseChkBox;
	CheckBox     *_arcadeStyleChkBox;
	string_t      _nameOrig;
	ConfControllerProfile _profile;

	float _time;
	int   _activeIndex;
	bool  _createNewProfile;
	bool  _skipNextKey;

	void OnSelectAction(int index);

public:
	ControlProfileDlg(Window *parent, const char *profileName);
	~ControlProfileDlg();

protected:
	void AddAction(ConfVarString &var, const string_t &display);

	void OnOK();
	void OnCancel();

	void OnTimeStep(float dt);
	bool OnRawChar(int c);
};

///////////////////////////////////////////////////////////////////////////////

class MapSettingsDlg : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, ComboBox> DefaultComboBox;
	DefaultComboBox *_theme;
	Edit *_author;
	Edit *_email;
	Edit *_url;
	Edit *_desc;
	Edit *_onInit;

public:
	MapSettingsDlg(Window *parent);
	~MapSettingsDlg();

	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
