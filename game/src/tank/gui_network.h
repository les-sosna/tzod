#pragma once
#include <ui/Dialog.h>
#include <memory>
#include <vector>

class World;
namespace FS
{
	class FileSystem;
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
	class ConsoleBuffer;
	template <class, class> class ListAdapter;
}
class ListDataSourceMaps;

class CreateServerDlg : public UI::Dialog
{
	typedef UI::ListAdapter<ListDataSourceMaps, UI::List> MapListBox;
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	MapListBox *_maps;
	UI::CheckBox  *_nightMode;
	UI::Edit      *_gameSpeed;
	UI::Edit      *_fragLimit;
	UI::Edit      *_timeLimit;
	UI::Edit      *_svFps;
//	UI::Edit      *_svLatency;

	DefaultComboBox *_lobbyList;
	UI::CheckBox  *_lobbyEnable;
	UI::Button    *_lobbyAdd;
    World &_world;
	FS::FileSystem &_fs;

public:
	CreateServerDlg(Window *parent, World &world, FS::FileSystem &fs);
	virtual ~CreateServerDlg();

protected:
	void OnOK();
	void OnCancel();
	void OnLobbyEnable();
	virtual void OnCloseChild(int result);
};

///////////////////////////////////////////////////////////////////////////////

class ConnectDlg
	: public UI::Dialog
//	, private IClientCallback
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	DefaultListBox *_status;
	UI::Button *_btnOK;
	UI::Edit   *_name;
//	std::unique_ptr<Subscribtion> _clientSubscribtion;
    World &_world;

public:
	ConnectDlg(UI::Window *parent, const std::string &defaultName, World &world);
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
};

///////////////////////////////////////////////////////////////////////////////

//class LobbyClient;
class InternetDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	DefaultListBox *_servers;
	UI::Button *_btnRefresh;
	UI::Button *_btnConnect;
	UI::Edit   *_name;
	UI::Text   *_status;
    World &_world;

public:
	InternetDlg(UI::Window *parent, World &world);
	virtual ~InternetDlg();

protected:
	void OnRefresh();
	void OnConnect();
	void OnCancel();
	void OnSelectServer(int idx);

	void OnCloseChild(int result);

	void OnLobbyError(const std::string &msg);
	void OnLobbyList(const std::vector<std::string> &result);

	void Error(const char *msg);

//	std::shared_ptr<LobbyClient> _client;
};

///////////////////////////////////////////////////////////////////////////////

class WaitingForPlayersDlg
	: public UI::Dialog
//	, private IClientCallback
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::List> DefaultListBox;
	DefaultListBox *_players;
	DefaultListBox *_bots;
	UI::Console    *_chat;
	UI::Button     *_btnOK;
	UI::Button     *_btnProfile;
	std::unique_ptr<UI::ConsoleBuffer>  _buf;
//	std::unique_ptr<Subscribtion> _clientSubscribtion;

	static const size_t _maxPings = 5;
	std::vector<unsigned int> _pings;
    World &_world;

public:
	WaitingForPlayersDlg(UI::Window *parent, World &world);
	virtual ~WaitingForPlayersDlg();

protected:
	// GUI event handlers
	void OnCloseProfileDlg(int result);
	void OnChangeProfileClick();
	void OnAddBotClick();
	void OnAddBotClose(int result);
	void OnOK();
	void OnCancel();
	void OnSendMessage(const std::string &msg);

private:
	// IClientCallback
	virtual void OnErrorMessage(const std::string &msg);
	virtual void OnTextMessage(const std::string &msg);
	virtual void OnPlayerReady(size_t playerIdx, bool ready);
	virtual void OnPlayersUpdate();
	virtual void OnStartGame();
	virtual void OnClientDestroy();
};
