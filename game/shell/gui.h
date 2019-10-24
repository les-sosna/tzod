#pragma once
#include "inc/shell/Config.h"
#include <ui/Dialog.h>

class LangCache;
class TextureManager;
namespace FS
{
	class FileSystem;
}

namespace Plat
{
	class ConsoleBuffer;
}

namespace UI
{
	class Button;
	class CheckBox;
	class ComboBox;
	class Edit;
	class ListBox;
	class ListDataSourceDefault;
	class Text;
	template<class, class> class ListAdapter;
}
class ListDataSourceMaps;

class NewGameDlg
	: public UI::Dialog
{
public:
	NewGameDlg(TextureManager &texman, FS::FileSystem &fs, ShellConfig &conf, Plat::ConsoleBuffer &logger, LangCache &lang);
	~NewGameDlg() override;

	bool OnKeyPressed(const Plat::Input &input, const UI::InputContext &ic, Plat::Key key) override;

protected:
	void RefreshPlayersList();
	void RefreshBotsList();

protected:
	void OnAddPlayer();
	void OnEditPlayer();
	void OnRemovePlayer();

	void OnAddBot();
	void OnEditBot();
	void OnRemoveBot();

	void OnOK();

private:
	typedef UI::ListAdapter<ListDataSourceMaps, UI::ListBox> MapList;
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ListBox> DefaultListBox;

	TextureManager &_texman;
	ShellConfig &_conf;
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
};

///////////////////////////////////////////////////////////////////////////////

class EditPlayerDlg : public UI::Dialog
{
public:
	EditPlayerDlg(TextureManager &texman, ConfVarTable &info, ShellConfig &conf, LangCache &lang);

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const Window &child) const override;

protected:
	void OnChangeSkin(int index);

	// Dialog
	bool OnClose(int result) override;

private:
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	std::shared_ptr<UI::Rectangle> _skinPreview;
	std::shared_ptr<UI::Edit> _name;
	std::shared_ptr<DefaultComboBox> _profiles;
	std::shared_ptr<DefaultComboBox> _skins;
	std::shared_ptr<DefaultComboBox> _classes;
	std::shared_ptr<DefaultComboBox> _teams;

	std::vector<std::pair<std::string, std::string>> _classNames;

	ConfPlayerLocal _info;
};

///////////////////////////////////////////////////////////////////////////////

class EditBotDlg : public UI::Dialog
{
public:
	EditBotDlg(TextureManager &texman, ConfVarTable &info, LangCache &lang);

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const Window &child) const override;

protected:
	void OnOK();
	void OnCancel();

	void OnChangeSkin(int index);

private:
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	std::shared_ptr<UI::Edit> _name;
	std::shared_ptr<UI::Rectangle> _skinPreview;
	std::shared_ptr<DefaultComboBox> _skins;
	std::shared_ptr<DefaultComboBox> _classes;
	std::shared_ptr<DefaultComboBox> _teams;
	std::shared_ptr<DefaultComboBox> _levels;

	std::vector<std::pair<std::string, std::string>> _classNames;

	ConfPlayerAI _info;
};
