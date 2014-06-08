// gui_desktop.cpp

#include "globals.h"
#include "gui_widgets.h"
#include "gui_desktop.h"
#include "gui_editor.h"
#include "gui_settings.h"
#include "gui_mainmenu.h"
#include "gui_scoretable.h"
#include "gui.h"

#include "video/TextureManager.h"

#include "config/Config.h"
#include "config/Language.h"
#include "core/Profiler.h"
#include "gc/Camera.h"
#include "gc/Player.h"
#include "gc/World.h"

//#include "network/TankClient.h"

#include "script.h"
#include "Macros.h"

#include <Console.h>
#include <ConsoleBuffer.h>
#include <GuiManager.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}


UI::ConsoleBuffer& GetConsole();


#include <GLFW/glfw3.h>


static CounterBase counterDt("dt", "dt, ms");


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

MessageArea::MessageArea(Window *parent, float x, float y)
  : Window(parent)
  , _fontTexture(GetManager()->GetTextureManager().FindSprite("font_small"))
{
	Move(x, y);
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

void MessageArea::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
	if( _lines.empty() || !g_conf.ui_showmsg.Get() )
	{
		return;
	}

	float h = GetManager()->GetTextureManager().GetCharHeight(_fontTexture);
	float y = std::max(_lines.front().time - 4.5f, 0.0f) * h * 2;
	for( LineList::const_iterator it = _lines.begin(); it != _lines.end(); ++it )
	{
		unsigned char cc = std::min(int(it->time * 255 * 2), 255);
		SpriteColor c;
		c.r = cc;
		c.g = cc;
		c.b = cc;
		c.a = cc;

		dc.DrawBitmapText(sx, sy + y, _fontTexture, c, it->str);
		y -= h;
	}
}

void MessageArea::WriteLine(const std::string &text)
{
	GetConsole().WriteLine(0, text);

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


void Desktop::MyConsoleHistory::Enter(const std::string &str)
{
	g_conf.con_history.PushBack(ConfVar::typeString)->AsStr()->Set(str);
	while( (signed) g_conf.con_history.GetSize() > g_conf.con_maxhistory.GetInt() )
	{
		g_conf.con_history.PopFront();
	}
}

size_t Desktop::MyConsoleHistory::GetItemCount() const
{
	return g_conf.con_history.GetSize();
}

const std::string& Desktop::MyConsoleHistory::GetItem(size_t index) const
{
	return g_conf.con_history.GetStr(index, "")->Get();
}

Desktop::Desktop(LayoutManager* manager, World &world)
  : Window(NULL, manager)
  , _font(GetManager()->GetTextureManager().FindSprite("font_default"))
  , _nModalPopups(0)
  , _world(world)
  , _worldView(*g_render, GetManager()->GetTextureManager())
{
	SetTexture("ui/window", false);
	_msg = new MessageArea(this, 100, 100);

	_editor = new EditorLayout(this, _world, _defaultCamera);
	_editor->SetVisible(false);

	_con = Console::Create(this, 10, 0, 100, 100, &GetConsole());
	_con->eventOnSendCommand = std::bind( &Desktop::OnCommand, this, std::placeholders::_1 );
	_con->eventOnRequestCompleteCommand = std::bind( &Desktop::OnCompleteCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
	_con->SetVisible(false);
	_con->SetTopMost(true);
	SpriteColor colors[] = {0xffffffff, 0xffff7fff};
	_con->SetColors(colors, sizeof(colors) / sizeof(colors[0]));
	_con->SetHistory(&_history);

	_score = new ScoreTable(this, _world);
	_score->SetVisible(false);


	_fps = new FpsCounter(this, 0, 0, alignTextLB, _world);
	g_conf.ui_showfps.eventChange = std::bind(&Desktop::OnChangeShowFps, this);
	OnChangeShowFps();

	_time = new TimeElapsed(this, 0, 0, alignTextRB, _world);
	g_conf.ui_showtime.eventChange = std::bind(&Desktop::OnChangeShowTime, this);
	OnChangeShowTime();

	if( g_conf.dbg_graph.Get() )
	{
		float xx = 200;
		float yy = 3;
		float hh = 50;
		for( size_t i = 0; i < CounterBase::GetMarkerCountStatic(); ++i )
		{
			Oscilloscope *os = new Oscilloscope(this, xx, yy);
			os->Resize(400, hh);
			os->SetRange(-1/15.0f, 1/15.0f);
			os->SetTitle(CounterBase::GetMarkerInfoStatic(i).title);
			CounterBase::SetMarkerCallbackStatic(i, CreateDelegate(&Oscilloscope::Push, os));
			yy += hh+5;
		}
	}
    
    assert(!world._messageListener);
    world._messageListener = this;

	OnRawChar(GLFW_KEY_ESCAPE); // to invoke main menu dialog
    SetTimeStep(true);
}

Desktop::~Desktop()
{
    assert(this == _world._messageListener);
    _world._messageListener = nullptr;
    
	g_conf.ui_showfps.eventChange = nullptr;
	g_conf.ui_showtime.eventChange = nullptr;
}
    
void Desktop::OnTimeStep(float dt)
{
	dt *= g_conf.sv_speed.GetFloat() / 100.0f;
    
//	if( g_client && (!IsGamePaused() || !g_client->SupportPause()) )
	{
		assert(dt >= 0);
		counterDt.Push(dt);
        
        _inputMgr.ReadControllerState(_world);
        _defaultCamera.HandleMovement(_world._sx, _world._sy, (float) GetWidth(), (float) GetHeight());
        _world.Step(dt);
	}
}

void Desktop::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
    if( _editor->GetVisible() || _world.GetList(LIST_cameras).empty() )
    {
		FRECT viewRect;
		viewRect.left = _defaultCamera.GetPosX();
		viewRect.top = _defaultCamera.GetPosY();
		viewRect.right = viewRect.left + (float) GetWidth() / _defaultCamera.GetZoom();
		viewRect.bottom = viewRect.top + (float) GetHeight() / _defaultCamera.GetZoom();
        
		g_render->Camera(NULL, _defaultCamera.GetPosX(), _defaultCamera.GetPosY(), _defaultCamera.GetZoom(), 0);
        _worldView.Render(_world, viewRect, _editor->GetVisible());
    }
    else
    {
		if( GetWidth() >= _world._sx && GetHeight() >= _world._sy )
		{
			// render from single camera with maximum shake
			float max_shake = -1;
			GC_Camera *singleCamera = NULL;
			FOREACH( _world.GetList(LIST_cameras), GC_Camera, pCamera )
			{
				if( pCamera->GetShake() > max_shake )
				{
					singleCamera = pCamera;
					max_shake = pCamera->GetShake();
				}
			}
			assert(singleCamera);
            
			FRECT viewRect;
			singleCamera->GetWorld(viewRect);
            
			Rect screen;
			singleCamera->GetScreen(screen);
            
			g_render->Camera(&screen,
                             viewRect.left,
                             viewRect.top,
                             singleCamera->GetZoom(),
                             g_conf.g_rotcamera.Get() ? singleCamera->GetAngle() : 0);
            
			_worldView.Render(_world, viewRect, false);
		}
		else
		{
			// render from each camera
			FOREACH( _world.GetList(LIST_cameras), GC_Camera, pCamera )
			{
				FRECT viewRect;
				pCamera->GetWorld(viewRect);
                
				Rect screen;
				pCamera->GetScreen(screen);
                
				g_render->Camera(&screen,
                                 viewRect.left,
                                 viewRect.top,
                                 pCamera->GetZoom(),
                                 g_conf.g_rotcamera.Get() ? pCamera->GetAngle() : 0);
                
				_worldView.Render(_world, viewRect, false);
			}
		}
    }
    
	g_render->SetMode(RM_INTERFACE);
	Window::DrawChildren(dc, sx, sy);
}

void Desktop::SetEditorMode(bool editorMode)
{
	_editor->SetVisible(editorMode);
    _world.PauseSound(IsGamePaused());
	if( editorMode && !_con->GetVisible() )
	{
		GetManager()->SetFocusWnd(_editor);
	}
}
    
bool Desktop::IsGamePaused() const
{
    return _nModalPopups > 0 || _world._limitHit || _editor->GetVisible();
}

void Desktop::ShowConsole(bool show)
{
	_con->SetVisible(show);
}

void Desktop::OnCloseChild(int result)
{
	SetDrawBackground(false);
    _nModalPopups--;
    _world.PauseSound(IsGamePaused());
}

MessageArea* Desktop::GetMsgArea() const
{
	return _msg;
}

bool Desktop::OnRawChar(int c)
{
	Dialog *dlg = NULL;

	switch( c )
	{
//	case GLFW_KEY_TAB:
//		_score->SetVisible(!_score->GetVisible());
//		break;

	case GLFW_KEY_GRAVE_ACCENT: // '~'
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

	case GLFW_KEY_ESCAPE:
		if( _con->Contains(GetManager()->GetFocusWnd()) )
		{
			_con->SetVisible(false);
		}
		else
		{
			dlg = new MainMenuDlg(this, _world, _inputMgr);
			SetDrawBackground(true);
			dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
            _nModalPopups++;
		}
		break;

	case GLFW_KEY_F2:
		dlg = new NewGameDlg(this, _world, _inputMgr);
		SetDrawBackground(true);
        dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
        _nModalPopups++;
		break;

	case GLFW_KEY_F12:
		dlg = new SettingsDlg(this, _world);
		SetDrawBackground(true);
        dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
        _nModalPopups++;
		break;

	case GLFW_KEY_F5:
        if (0 == _nModalPopups)
            SetEditorMode(!_editor->GetVisible());
		break;

	case GLFW_KEY_F8:
		if( _editor->GetVisible() ) // TODO: move to editor layout
		{
			dlg = new MapSettingsDlg(this, _world);
			SetDrawBackground(true);
			dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
            _nModalPopups++;
		}
		break;

	default:
		return false;
	}

	return true;
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
    GC_Camera::UpdateLayout(_world, width, height);
}

void Desktop::OnChangeShowFps()
{
	_fps->SetVisible(g_conf.ui_showfps.Get());
}

void Desktop::OnChangeShowTime()
{
	_fps->SetVisible(g_conf.ui_showfps.Get());
}

void Desktop::OnCommand(const std::string &cmd)
{
	if( cmd.empty() )
	{
		return;
	}

	std::string exec;

    if( cmd[0] == '/' )
    {
        exec = cmd.substr(1); // cut off the first symbol and execute cmd
    }
    else
    {
//        dynamic_cast<TankClient*>(g_client)->SendTextMessage(cmd);
        return;
    }


	if( luaL_loadstring(g_env.L, exec.c_str()) )
	{
		lua_pop(g_env.L, 1);

		std::string tmp = "print(";
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

bool Desktop::OnCompleteCommand(const std::string &cmd, int &pos, std::string &result)
{
	assert(pos >= 0);
	lua_getglobal(g_env.L, "autocomplete");
	if( lua_isnil(g_env.L, -1) )
	{
		lua_pop(g_env.L, 1);
		GetConsole().WriteLine(1, "There was no autocomplete module loaded");
		return false;
	}
	lua_pushlstring(g_env.L, cmd.substr(0, pos).c_str(), pos);
	if( lua_pcall(g_env.L, 1, 1, 0) )
	{
		GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
        lua_pop(g_env.L, 1); // pop error message
	}
	else
	{
		const char *str = lua_tostring(g_env.L, -1);
		std::string insert = str ? str : "";

		result = cmd.substr(0, pos) + insert + cmd.substr(pos);
		pos += insert.length();

		if( !result.empty() && result[0] != '/' )
		{
			result = std::string("/") + result;
			++pos;
		}
	}
	lua_pop(g_env.L, 1); // pop result or error message
	return true;
}
    
void Desktop::OnGameMessage(const char *msg)
{
    _msg->WriteLine(msg);
}

} // end of namespace UI

// end of file
