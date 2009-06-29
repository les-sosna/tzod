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
#include "video/TextureManager.h"

#include "config/Config.h"
#include "core/Console.h"
#include "core/Application.h"

#include "network/TankClient.h"

#include "Level.h"
#include "script.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

MessageArea::MessageArea(Window *parent, float x, float y)
  : Window(parent, x, y, NULL)
  , _fontTexture(g_texman->FindSprite("font_small"))
{
}

MessageArea::~MessageArea()
{
}

void MessageArea::OnTimeStep(float dt)
{
	for( size_t i = 0; i < _lines.size(); ++i )
		_lines[i].time -= dt;
	while( !_lines.empty() && _lines.back().time <= 0 )
		_lines.pop_back();

	if( _lines.empty() )
	{
		SetTimeStep(false);
		return;
	}
}

void MessageArea::DrawChildren(float sx, float sy) const
{
	if( _lines.empty() || !g_conf->ui_showmsg->Get() )
	{
		return;
	}

	float h = g_texman->GetCharHeight(_fontTexture);
	float y = std::max(_lines.front().time - 4.5f, 0.0f) * h * 2;
	for( LineList::const_iterator it = _lines.begin(); it != _lines.end(); ++it )
	{
		unsigned char cc = std::min(int(it->time * 255 * 2), 255);
		SpriteColor c;
		c.r = cc;
		c.g = cc;
		c.b = cc;
		c.a = cc;

		g_texman->DrawBitmapText(_fontTexture, it->str, c, sx, sy + y);
		y -= h;
	}
}

void MessageArea::WriteLine(const string_t &text)
{
	g_app->GetConsole()->puts(text.c_str());
	g_app->GetConsole()->puts("\n");

	Line line;
	line.time = 5;  // timeout
	line.str = text;
	_lines.push_front(line);

	SetTimeStep(true);
}

void MessageArea::Clear()
{
	_lines.clear();
	SetTimeStep(false);
}

///////////////////////////////////////////////////////////////////////////////

Desktop::Desktop(GuiManager* manager)
  : Window(manager)
{
	_msg = new MessageArea(this, 100, 100);

	_editor = new EditorLayout(this);
	_editor->SetVisible(false);

	_con = new Console(this, 10, 0, 100, 100, g_app->GetConsole());
	_con->eventOnSendCommand.bind( &Desktop::OnCommand, this );
	_con->eventOnRequestCompleteCommand.bind( &Desktop::OnCompleteCommand, this );
	_con->SetVisible(false);

	_score = new ScoreTable(this);
	_score->SetVisible(false);


	_fps = new FpsCounter(this, 0, 0, alignTextLB);
	g_conf->ui_showfps->eventChange.bind( &Desktop::OnChangeShowFps, this );
	OnChangeShowFps();

	_time = new TimeElapsed(this, 0, 0, alignTextRB);
	g_conf->ui_showtime->eventChange.bind( &Desktop::OnChangeShowTime, this );
	OnChangeShowTime();

	_oscill = new Oscilloscope(this, 100, 5);
	_oscill->Resize(400, 45);
	_oscill->SetRange(-1/30.0f, 1/10.0f);
	_oscill->SetTitle("timebuf");
	float grid[] = {0};
	_oscill->SetGrid(grid, sizeof(grid) / sizeof(grid[0]));

	_oscill2 = new Oscilloscope(this, 100, 55);
	_oscill2->Resize(400, 45);
	_oscill2->SetRange(0, 1/30.0f);
	_oscill2->SetTitle("jitter");
	float grid2[] = {1/60.0f};
	_oscill2->SetGrid(grid2, sizeof(grid2) / sizeof(grid2[0]));

	_oscill3 = new Oscilloscope(this, 100, 105);
	_oscill3->Resize(400, 45);
	_oscill3->SetRange(0, 3);
	_oscill3->SetTitle("steps");
	float grid3[] = {1, 2};
	_oscill3->SetGrid(grid3, sizeof(grid3) / sizeof(grid3[0]));

	_oscill4 = new Oscilloscope(this, 100, 155);
	_oscill4->Resize(400, 45);
	_oscill4->SetRange(0, 10);
	_oscill4->SetTitle("client latency");
	float grid4[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	_oscill4->SetGrid(grid4, sizeof(grid4) / sizeof(grid4[0]));


	OnRawChar(VK_ESCAPE); // to invoke main menu dialog
}

Desktop::~Desktop()
{
	g_conf->ui_showfps->eventChange.clear();
	g_conf->ui_showtime->eventChange.clear();
}

void Desktop::ShowDesktopBackground(bool show)
{
	SetTexture(show ? "ui/window" : NULL);
}

void Desktop::ShowConsole(bool show)
{
	_con->SetVisible(show);
}

void Desktop::ShowEditor(bool show)
{
	assert(!show || g_level);
	_editor->SetVisible(show);
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
//	case VK_TAB:
//		_score->SetVisible(!_score->GetVisible());
//		break;

	case VK_OEM_3: // '~'
		if( _con->GetVisible() )
		{
			_con->SetVisible(false);
		}
		else
		{
			_con->SetVisible(true);
			GetManager()->SetFocusWnd(_con);
		}
		break;

	case VK_ESCAPE:
		if( GetManager()->GetFocusWnd() && GetManager()->Unfocus(_con) )
		{
			_con->SetVisible(false);
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
		if( !g_level->IsEmpty() )
		{
			g_level->ToggleEditorMode();
			ShowEditor(g_level->_modeEditor);
		}
		break;

	case VK_F8:
		if( g_level->_modeEditor )
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
	_msg->Move(_msg->GetX(), GetHeight() - 50);
}

void Desktop::OnChangeShowFps()
{
	_fps->SetVisible(g_conf->ui_showfps->Get());
}

void Desktop::OnChangeShowTime()
{
	_fps->SetVisible(g_conf->ui_showfps->Get());
}

void Desktop::OnCommand(const string_t &cmd)
{
	if( cmd.empty() )
	{
		return;
	}

	string_t exec;

	if( g_client )
	{
		if( cmd[0] == '/' )
		{
			exec = cmd.substr(1); // cut off the first symbol and execute cmd
		}
		else
		{
			g_client->SendTextMessage(cmd);
			return;
		}
	}
	else
	{
		exec = cmd;
	}


	if( luaL_loadstring(g_env.L, exec.c_str()) )
	{
		lua_pop(g_env.L, 1);

		string_t tmp = "print(";
		tmp += exec;
		tmp += ")";

		if( luaL_loadstring(g_env.L, tmp.c_str()) )
		{
			lua_pop(g_env.L, 1);
		}
		else
		{
			script_exec(g_env.L, tmp.c_str());
			return;
		}
	}

	script_exec(g_env.L, exec.c_str());
}

bool Desktop::OnCompleteCommand(const string_t &cmd, string_t &result)
{
	lua_getglobal(g_env.L, "autocomplete");
	if( lua_isnil(g_env.L, -1) )
	{
		lua_pop(g_env.L, 1);
		g_app->GetConsole()->printf("There was no autocomplete module loaded\n");
		return false;
	}
	lua_pushlstring(g_env.L, cmd.c_str(), cmd.length());
	HRESULT hr = S_OK;
	if( lua_pcall(g_env.L, 1, 1, 0) )
	{
		g_app->GetConsole()->printf("%s\n", lua_tostring(g_env.L, -1));
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
