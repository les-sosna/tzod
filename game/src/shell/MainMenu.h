#pragma once
#include <ui/Window.h>
#include <functional>

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ConsoleBuffer;
	class Text;
}

class ConfCache;
class GetFileNameDlg;

struct MainMenuCommands
{
	std::function<void()> newCampaign;
	std::function<void()> newDM;
	std::function<void()> newMap;
	std::function<void(std::string)> openMap;
	std::function<void(std::string)> exportMap;
	std::function<void()> gameSettings;
	std::function<void()> close;
};

class MainMenuDlg : public UI::Window
{
	void OnEditor();
	void OnMapSettings();
	void OnImportMap();
	void OnImportMapSelect(int result);
	void OnExportMap();
	void OnExportMapSelect(int result);

	void OnSettings();


	enum PanelType
	{
		PT_NONE,
		PT_EDITOR,
	};

	enum PanelState
	{
		PS_NONE,
		PS_APPEARING,
		PS_DISAPPEARING,
	};

	UI::Window    *_panel = nullptr;
	UI::Window    *_panelFrame = nullptr;
	UI::Text      *_panelTitle = nullptr;
	PanelType  _ptype;
	PanelState _pstate;

	GetFileNameDlg *_fileDlg;
	FS::FileSystem &_fs;
	LangCache &_lang;
	UI::ConsoleBuffer &_logger;
	MainMenuCommands _commands;

public:
	MainMenuDlg(Window *parent,
	            FS::FileSystem &fs,
	            ConfCache &conf,
	            LangCache &lang,
	            UI::ConsoleBuffer &logger,
	            MainMenuCommands commands);
	virtual ~MainMenuDlg();
	bool OnKeyPressed(UI::Key key) override;
	bool OnFocus(bool) override { return true; }

protected:
	void OnTimeStep(float dt) override;
	void OnCloseChild(int result);
	void CreatePanel(); // create panel of current _ptype and go to PS_APPEARING state
	void SwitchPanel(PanelType newtype);
};
