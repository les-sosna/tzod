// gui_desktop.cpp

#include "stdafx.h"

#include "gui_widgets.h"
#include "gui_desktop.h"
#include "gui_editor.h"
#include "gui_settings.h"
#include "gui_mainmenu.h"
#include "gui_scoretable.h"
#include "gui.h"
#include "Console.h"

#include "GuiManager.h"

#include "config/Config.h"
#include "core/Console.h"

#include "network/TankClient.h"

#include "Level.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

MessageArea::MessageArea(Window *parent, float x, float y) : Window(parent, x, y, NULL)
{
	_text = new Text(this, 0, 0, "", alignTextLT);
}

void MessageArea::OnTimeStep(float dt)
{
	for( size_t i = 0; i < _lines.size(); ++i )
		_lines[i].time += dt;
	while( !_lines.empty() && _lines.front().time > 5 )
		_lines.pop_front();

	if( _lines.empty() )
	{
		SetTimeStep(false);
		_text->Show(false);
		return;
	}

	string_t str;
	for( size_t i = 0; i < _lines.size(); ++i )
		str.append(_lines[i].str);
	_text->SetText(str.c_str());
}

void MessageArea::puts(const char *text)
{
	Line line;
	line.time = 0;
	line.str = text;
	line.str.append("\n");
	_lines.push_back(line);
	g_console->puts(line.str.c_str());

	SetTimeStep(true);
	_text->Show(true);
}

void MessageArea::Clear()
{
	_lines.clear();
	SetTimeStep(false);
	_text->SetText("");
	_text->Show(false);
}

///////////////////////////////////////////////////////////////////////////////

Desktop::Desktop(GuiManager* manager) : Window(manager)
{
	_msg = new MessageArea(this, 100, 100);

	_editor = new EditorLayout(this);
	_editor->Show(false);

	_con = new Console(this, 10, 0, 100, 100, g_console);
	_con->eventOnSendCommand.bind( &Desktop::OnCommand, this );
	_con->eventOnRequestCompleteCommand.bind( &Desktop::OnCompleteCommand, this );
	_con->Show(false);

	_score = new ScoreTable(this);
	_score->Show(false);


	_fps = new FpsCounter(this, 0, 0, alignTextLB);
	g_conf.ui_showfps->eventChange.bind( &Desktop::OnChangeShowFps, this );
	OnChangeShowFps();

	_time = new TimeElapsed(this, 0, 0, alignTextRB );
	g_conf.ui_showtime->eventChange.bind( &Desktop::OnChangeShowTime, this );
	OnChangeShowTime();

	OnRawChar(VK_ESCAPE); // to invoke main menu dialog
}

Desktop::~Desktop()
{
	g_conf.ui_showfps->eventChange.clear();
	g_conf.ui_showtime->eventChange.clear();
}

void Desktop::ShowDesktopBackground(bool show)
{
	SetTexture(show ? "window" : NULL);
}

void Desktop::ShowConsole(bool show)
{
	_con->Show(show);
}

void Desktop::ShowEditor(bool show)
{
	_ASSERT(!show || g_level);
	_editor->Show(show);
	if( show )
	{
		GetManager()->SetFocusWnd(_editor);
	}
}

void Desktop::OnCloseChild(int result)
{
	ShowDesktopBackground(false);
}

MessageArea* Desktop::GetMsgArea() const
{
	return _msg;
}

void Desktop::OnRawChar(int c)
{
	Dialog *dlg = NULL;

	switch( c )
	{
	case VK_TAB:
		_score->Show(!_score->IsVisible());
		break;

	case VK_OEM_3: // '~'
		if( _con->IsVisible() )
		{
			_con->Show(false);
		}
		else
		{
			_con->Show(true);
			GetManager()->SetFocusWnd(_con);
		}
		break;

	case VK_ESCAPE:
		if( GetManager()->GetFocusWnd() && GetManager()->Unfocus(_con) )
		{
			_con->Show(false);
		}
		else
		{
			dlg = new MainMenuDlg(this);
			ShowDesktopBackground(true);
			dlg->eventClose.bind( &Desktop::OnCloseChild, this );
		}
		break;

	case VK_F2:
		dlg = new NewGameDlg(this);
		ShowDesktopBackground(true);
		dlg->eventClose.bind( &Desktop::OnCloseChild, this );
		break;

	case VK_F12:
		dlg = new SettingsDlg(this);
		ShowDesktopBackground(true);
		dlg->eventClose.bind( &Desktop::OnCloseChild, this );
		break;

	case VK_F5:
		if( g_level )
		{
			g_level->ToggleEditorMode();
			ShowEditor(g_level->_modeEditor);
		}
		break;

	case VK_F8:
		if( g_level && g_level->_modeEditor )
		{
			dlg = new MapSettingsDlg(this);
			ShowDesktopBackground(true);
			dlg->eventClose.bind( &Desktop::OnCloseChild, this );
		}
		break;
	}
}

bool Desktop::OnFocus(bool focus)
{
	return true;
}

void Desktop::OnSize(float width, float height)
{
	_editor->Resize(GetWidth(), GetHeight());
	_con->Resize(GetWidth() - 20, floorf(GetHeight() * 0.5f + 0.5f));
	_fps->Move(1, GetHeight() - 1);
	_time->Move( GetWidth() - 1, GetHeight() - 1 );
}

void Desktop::OnChangeShowFps()
{
	_fps->Show(g_conf.ui_showfps->Get());
}

void Desktop::OnChangeShowTime()
{
	_fps->Show(g_conf.ui_showfps->Get());
}

void Desktop::OnCommand(const char *cmd)
{
	size_t len = strlen(cmd);

	if( g_client )
	{
		if( cmd[0] != '/' )
		{
			DataBlock db(len + 1);
			strcpy((char*) db.data(), cmd);
			db.type() = DBTYPE_TEXTMESSAGE;
			g_client->SendDataToServer(db);
		}
		else if( len > 1 )
		{
			script_exec(g_env.L, cmd + 1); // cut off first symbol '/'
		}
	}
	else
	{
		script_exec(g_env.L, cmd);
	}
}

bool Desktop::OnCompleteCommand(const char *cmd, string_t &result)
{
	lua_getglobal(g_env.L, "autocomplete");
	if( lua_isnil(g_env.L, -1) )
	{
		lua_pop(g_env.L, 1);
		g_console->printf("There is no autocomplete module is loaded\n");
		return false;
	}
	lua_pushstring(g_env.L, cmd);
	HRESULT hr = S_OK;
	if( lua_pcall(g_env.L, 1, 1, 0) )
	{
		g_console->printf("%s\n", lua_tostring(g_env.L, -1));
	}
	else
	{
		const char *str = lua_tostring(g_env.L, -1);
		result = str ? str : "";
	}
	lua_pop(g_env.L, 1); // pop result
	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
