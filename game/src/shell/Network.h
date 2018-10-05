#pragma once
#include <ui/Dialog.h>
#include <memory>
#include <vector>

class LangCache;
class World;
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
	class Text;
	class List;
	class ListDataSourceDefault;
	class Edit;
	class ComboBox;
	class CheckBox;
	class Button;
	class Console;
	template <class, class> class ListAdapter;
}
class ListDataSourceMaps;

class CreateServerDlg : public UI::Dialog
{
public:
	CreateServerDlg(UI::TimeStepManager &manager, TextureManager &texman, World &world, FS::FileSystem &fs, ShellConfig &conf, LangCache &lang, Plat::ConsoleBuffer &logger);
	virtual ~CreateServerDlg();

protected:
	void OnOK();
	void OnCancel();
	void OnLobbyEnable();
	virtual void OnCloseChild(std::shared_ptr<UI::Dialog> sender, int result);

private:
	typedef UI::ListAdapter<ListDataSourceMaps, UI::List> MapListBox;
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	std::shared_ptr<MapListBox> _maps;
	std::shared_ptr<UI::CheckBox> _nightMode;
	std::shared_ptr<UI::Edit> _gameSpeed;
	std::shared_ptr<UI::Edit> _fragLimit;
	std::shared_ptr<UI::Edit> _timeLimit;
	std::shared_ptr<UI::Edit> _svFps;
	//	std::shared_ptr<UI::Edit> _svLatency;

	std::shared_ptr<DefaultComboBox> _lobbyList;
	std::shared_ptr<UI::CheckBox> _lobbyEnable;
	std::shared_ptr<UI::Button> _lobbyAdd;
	World &_world;
	FS::FileSystem &_fs;
	ShellConfig &_conf;
	LangCache &_lang;
	Plat::ConsoleBuffer &_logger;
};

///////////////////////////////////////////////////////////////////////////////

class ConnectDlg
	: public UI::Dialog
//	, private IClientCallback
{
public:
	ConnectDlg(UI::TimeStepManager &manager, TextureManager &texman, const std::string &defaultName, World &world, ShellConfig &conf, LangCache &lang);
	virtual ~ConnectDlg();

protected:
	void OnOK();
	void OnCancel();

private:
	// IClientCallback
	virtual void OnConnected();
	virtual void OnErrorMessage(const std::string &msg);
	virtual void OnTextMessage(const std::string &msg);
	virtual void OnClientDestroy();

private:
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	std::shared_ptr<DefaultListBox> _status;
	std::shared_ptr<UI::Button> _btnOK;
	std::shared_ptr<UI::Edit>   _name;
	//	std::unique_ptr<Subscribtion> _clientSubscribtion;
	World &_world;
	ShellConfig &_conf;
	LangCache &_lang;
};

///////////////////////////////////////////////////////////////////////////////

//class LobbyClient;
class InternetDlg : public UI::Dialog
{
public:
	InternetDlg(UI::TimeStepManager &manager, TextureManager &texman, World &world, ShellConfig &conf, LangCache &lang);
	virtual ~InternetDlg();

protected:
	void OnRefresh();
	void OnConnect();
	void OnCancel();
	void OnSelectServer(int idx);

	void OnCloseChild(std::shared_ptr<UI::Dialog> sender, int result);

	void OnLobbyError(const std::string &msg);
	void OnLobbyList(const std::vector<std::string> &result);

	void Error(const char *msg);

//	std::shared_ptr<LobbyClient> _client;

private:
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	std::shared_ptr<DefaultListBox> _servers;
	std::shared_ptr<UI::Button> _btnRefresh;
	std::shared_ptr<UI::Button> _btnConnect;
	std::shared_ptr<UI::Edit>   _name;
	std::shared_ptr<UI::Text>   _status;
	World &_world;
	ShellConfig &_conf;
	LangCache &_lang;
};

///////////////////////////////////////////////////////////////////////////////

class WaitingForPlayersDlg
	: public UI::Dialog
//	, private IClientCallback
{
public:
	WaitingForPlayersDlg(UI::TimeStepManager &manager, TextureManager &texman, World &world, ShellConfig &conf, LangCache &lang);
	virtual ~WaitingForPlayersDlg();

protected:
	// GUI event handlers
	void OnCloseProfileDlg(std::shared_ptr<UI::Dialog> sender, int result);
	void OnChangeProfileClick();
	void OnAddBotClick();
	void OnAddBotClose(std::shared_ptr<UI::Dialog> sender, int result);
	void OnOK();
	void OnCancel();
	void OnSendMessage(const std::string &msg);

private:
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	std::shared_ptr<DefaultListBox> _players;
	std::shared_ptr<DefaultListBox> _bots;
	std::shared_ptr<UI::Console>    _chat;
	std::shared_ptr<UI::Button>     _btnOK;
	std::shared_ptr<UI::Button>     _btnProfile;
	std::unique_ptr<Plat::ConsoleBuffer>  _buf;
	//	std::unique_ptr<Subscribtion> _clientSubscribtion;

	static const size_t _maxPings = 5;
	std::vector<unsigned int> _pings;
	World &_world;
	ShellConfig &_conf;
	LangCache &_lang;

	// IClientCallback
	virtual void OnErrorMessage(const std::string &msg);
	virtual void OnTextMessage(const std::string &msg);
	virtual void OnPlayerReady(size_t playerIdx, bool ready);
	virtual void OnPlayersUpdate();
	virtual void OnStartGame();
	virtual void OnClientDestroy();
};

