#pragma once

#include "constants.h"
#include "Config.h"
#include <ui/Dialog.h>

class ConfVarTable;
namespace FS
{
	class FileSystem;
}

namespace UI
{
	class Button;
	class CheckBox;
	class ComboBox;
	class Edit;
	class List;
	class ListDataSourceDefault;
	class Text;
	template<class, class> class ListAdapter;
}
class ListDataSourceMaps;

class NewGameDlg : public UI::Dialog
{
	typedef UI::ListAdapter<ListDataSourceMaps, UI::List> MapList;
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;

	MapList      *_maps;
	DefaultListBox  *_players;
	DefaultListBox  *_bots;
	UI::CheckBox  *_nightMode;
	UI::Edit      *_gameSpeed;
	UI::Edit      *_fragLimit;
	UI::Edit      *_timeLimit;

	UI::Button    *_removePlayer;
	UI::Button    *_changePlayer;
	UI::Button    *_removeBot;
	UI::Button    *_changeBot;

	bool _newPlayer;

public:
	NewGameDlg(Window *parent, FS::FileSystem &fs);
	virtual ~NewGameDlg();

	virtual bool OnRawChar(int c);

protected:
	void RefreshPlayersList();
	void RefreshBotsList();

protected:
	void OnAddPlayer();
	void OnAddPlayerClose(int result);
	void OnEditPlayer();
	void OnEditPlayerClose(int result);
	void OnRemovePlayer();
	void OnSelectPlayer(int index);

	void OnAddBot();
	void OnAddBotClose(int result);
	void OnEditBot();
	void OnEditBotClose(int result);
	void OnRemoveBot();
	void OnSelectBot(int index);

	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////

class EditPlayerDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	UI::Window   *_skinPreview;
	UI::Edit     *_name;
	DefaultComboBox *_profiles;
	DefaultComboBox *_skins;
	DefaultComboBox *_classes;
	DefaultComboBox *_teams;

	std::vector<std::pair<std::string, std::string> > _classNames;

	ConfPlayerLocal _info;

public:
	EditPlayerDlg(UI::Window *parent, ConfVarTable *info);

protected:
	void OnOK();
	void OnCancel();

	void OnChangeSkin(int index);
};

///////////////////////////////////////////////////////////////////////////////

class EditBotDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	UI::Edit     *_name;
	UI::Window   *_skinPreview;
	DefaultComboBox *_skins;
	DefaultComboBox *_classes;
	DefaultComboBox *_teams;
	DefaultComboBox *_levels;

	std::vector<std::pair<std::string, std::string> > _classNames;

	ConfPlayerAI _info;

public:
	EditBotDlg(Window *parent, ConfVarTable *info);

	static const char levels[AI_MAX_LEVEL+1][16];

protected:
	void OnOK();
	void OnCancel();

	void OnChangeSkin(int index);
};

///////////////////////////////////////////////////////////////////////////////

class ScriptMessageBox : public UI::Window
{
	UI::Text   *_text;
	UI::Button *_button1;
	UI::Button *_button2;
	UI::Button *_button3;

	void OnButton1();
	void OnButton2();
	void OnButton3();

public:
	ScriptMessageBox(UI::Window *parent,
		const std::string &title,
		const std::string &text,
		const std::string &btn1,
		const std::string &btn2,
		const std::string &btn3
	);
	std::function<void(int)> eventSelect;
};

