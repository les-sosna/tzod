#pragma once
#include "inc/shell/Config.h"
#include <ui/Dialog.h>

class LangCache;
class TextureManager;
namespace FS
{
	class FileSystem;
}

namespace UI
{
	class Button;
	class CheckBox;
	class ComboBox;
	class ConsoleBuffer;
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

	TextureManager &_texman;
	ConfCache &_conf;
	LangCache &_lang;
	std::shared_ptr<MapList> _maps;
	std::shared_ptr<DefaultListBox> _players;
	std::shared_ptr<DefaultListBox> _bots;
	std::shared_ptr<UI::CheckBox> _nightMode;
	std::shared_ptr<UI::Edit> _gameSpeed;
	std::shared_ptr<UI::Edit> _fragLimit;
	std::shared_ptr<UI::Edit> _timeLimit;

	std::shared_ptr<UI::Button> _removePlayer;
	std::shared_ptr<UI::Button> _changePlayer;
	std::shared_ptr<UI::Button> _removeBot;
	std::shared_ptr<UI::Button> _changeBot;

	bool _newPlayer;

public:
	NewGameDlg(UI::LayoutManager &manager, TextureManager &texman, FS::FileSystem &fs, ConfCache &conf, UI::ConsoleBuffer &logger, LangCache &lang);
	~NewGameDlg() override;

	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;

protected:
	void RefreshPlayersList();
	void RefreshBotsList();

protected:
	void OnAddPlayer();
	void OnAddPlayerClose(std::shared_ptr<UI::Dialog> sender, int result);
	void OnEditPlayer();
	void OnEditPlayerClose(std::shared_ptr<UI::Dialog> sender, int result);
	void OnRemovePlayer();
	void OnSelectPlayer(int index);

	void OnAddBot();
	void OnAddBotClose(std::shared_ptr<UI::Dialog> sender, int result);
	void OnEditBot();
	void OnEditBotClose(std::shared_ptr<UI::Dialog> sender, int result);
	void OnRemoveBot();
	void OnSelectBot(int index);

	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////

class EditPlayerDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	std::shared_ptr<UI::Rectangle> _skinPreview;
	std::shared_ptr<UI::Edit> _name;
	std::shared_ptr<DefaultComboBox> _profiles;
	std::shared_ptr<DefaultComboBox> _skins;
	std::shared_ptr<DefaultComboBox> _classes;
	std::shared_ptr<DefaultComboBox> _teams;

	std::vector<std::pair<std::string, std::string>> _classNames;

	ConfPlayerLocal _info;

public:
	EditPlayerDlg(UI::LayoutManager &manager, TextureManager &texman, ConfVarTable &info, ConfCache &conf, LangCache &lang);

protected:
	void OnChangeSkin(int index);

	// Dialog
	bool OnClose(int result) override;
};

///////////////////////////////////////////////////////////////////////////////

class EditBotDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	std::shared_ptr<UI::Edit> _name;
	std::shared_ptr<UI::Rectangle> _skinPreview;
	std::shared_ptr<DefaultComboBox> _skins;
	std::shared_ptr<DefaultComboBox> _classes;
	std::shared_ptr<DefaultComboBox> _teams;
	std::shared_ptr<DefaultComboBox> _levels;

	std::vector<std::pair<std::string, std::string>> _classNames;

	ConfPlayerAI _info;

public:
	EditBotDlg(UI::LayoutManager &manager, TextureManager &texman, ConfVarTable &info, LangCache &lang);

protected:
	void OnOK();
	void OnCancel();

	void OnChangeSkin(int index);
};

///////////////////////////////////////////////////////////////////////////////

class ScriptMessageBox : public UI::Rectangle
{
	std::shared_ptr<UI::Text> _text;
	std::shared_ptr<UI::Button> _button1;
	std::shared_ptr<UI::Button> _button2;
	std::shared_ptr<UI::Button> _button3;

	void OnButton1();
	void OnButton2();
	void OnButton3();

public:
	ScriptMessageBox(
		UI::LayoutManager &manager,
		TextureManager &texman,
		const std::string &title,
		const std::string &text,
		const std::string &btn1,
		const std::string &btn2,
		const std::string &btn3
	);
	std::function<void(int)> eventSelect;
};

