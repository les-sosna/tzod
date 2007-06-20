// gui_settings.h

#pragma once

#include "ui/Base.h"
#include "ui/Dialog.h"

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
} // end of namespace UI

// end of file
