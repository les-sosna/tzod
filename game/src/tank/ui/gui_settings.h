// gui_settings.h

#pragma once

#include "ui/Base.h"
#include "ui/Dialog.h"


// forward declaration
class ConfVarTable;


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class SettingsDlg : public Dialog
{
	List      *_profiles;
	Button    *_editProfile;
	Button    *_deleteProfile;

	CheckBox  *_particles;
	CheckBox  *_showFps;
	CheckBox  *_showTime;
	CheckBox  *_showDamage;

	ScrollBar *_volume;


public:
	SettingsDlg(Window *parent);
	~SettingsDlg();

protected:
	void OnAddProfile();
	void OnEditProfile();
	void OnDeleteProfile();
	void OnSelectProfile(int index);

	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////

class ControlProfileDlg : public Dialog
{
	List         *_actions;
	ConfVarTable *_profile;

public:
	ControlProfileDlg(Window *parent, ConfVarTable *profile);
	~ControlProfileDlg();

protected:
	void AddAction(const char *rawname, const char *display);

	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
