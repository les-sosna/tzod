#pragma once
#include <ui/Dialog.h>

namespace UI
{
	template <class, class> class ListAdapter;
	class ComboBox;
	class Edit;
	class ListDataSourceDefault;
}

class ThemeManager;
class World;

class MapSettingsDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;
	DefaultComboBox *_theme;
	UI::Edit *_author;
	UI::Edit *_email;
	UI::Edit *_url;
	UI::Edit *_desc;
	UI::Edit *_onInit;
	World &_world;

public:
	MapSettingsDlg(UI::Window *parent, World &world, const ThemeManager &themeManager);
	~MapSettingsDlg();

	void OnOK();
	void OnCancel();
};
