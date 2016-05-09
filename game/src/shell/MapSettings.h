#pragma once
#include <ui/Dialog.h>

namespace UI
{
	template <class, class> class ListAdapter;
	class ComboBox;
	class Edit;
	class ListDataSourceDefault;
}

class LangCache;
class ThemeManager;
class World;

class MapSettingsDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;
	std::shared_ptr<DefaultComboBox> _theme;
	std::shared_ptr<UI::Edit> _author;
	std::shared_ptr<UI::Edit> _email;
	std::shared_ptr<UI::Edit> _url;
	std::shared_ptr<UI::Edit> _desc;
	std::shared_ptr<UI::Edit> _onInit;
	World &_world;

public:
	MapSettingsDlg(UI::LayoutManager &manager, World &world, const ThemeManager &themeManager, LangCache &lang);
	~MapSettingsDlg();

	void OnOK();
	void OnCancel();
};
