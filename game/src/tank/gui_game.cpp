#include "Config.h"
#include "Controller.h"
#include "DefaultCamera.h"
#include "gui_game.h"
#include "gui_messagearea.h"
#include "gui_scoretable.h"
#include "InputManager.h"

#include <app/Deathmatch.h>
#include <app/GameContext.h>
#include <app/WorldController.h>
#include <gc/Macros.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <ui/GuiManager.h>
#include <ui/UIInput.h>
#include <video/DrawingContext.h>

#include <GLFW/glfw3.h>
#include <sstream>

UI::TimeElapsed::TimeElapsed(Window *parent, float x, float y, enumAlignText align, World &world)
  : Text(parent)
  , _world(world)
{
	SetTimeStep(true);
	Move(x, y);
	SetAlign(align);
}

void UI::TimeElapsed::OnVisibleChange(bool visible, bool inherited)
{
	SetTimeStep(visible);
}

void UI::TimeElapsed::OnTimeStep(float dt)
{
	std::ostringstream text;
	int time = (int) _world.GetTime();
	text << (time / 60) << ":";
	if( time % 60 < 10 )
		text << "0";
	text << (time % 60);
	SetText(text.str());
}

///////////////////////////////////////////////////////////////////////////////

UI::GameLayout::GameLayout(Window *parent,
						   GameContext &gameContext,
						   WorldView &worldView,
						   WorldController &worldController,
						   const DefaultCamera &defaultCamera)
    : Window(parent)
    , _gameContext(gameContext)
    , _gameViewHarness(gameContext.GetWorld())
    , _worldView(worldView)
	, _worldController(worldController)
    , _defaultCamera(defaultCamera)
    , _inputMgr(gameContext.GetWorld())
{
	_msg = new MessageArea(this, 100, 100);
    
	_score = new ScoreTable(this, _gameContext.GetWorld(), _gameContext.GetGameplay());
	_score->SetVisible(false);
    
	_time = new TimeElapsed(this, 0, 0, alignTextRB, _gameContext.GetWorld());
	g_conf.ui_showtime.eventChange = std::bind(&GameLayout::OnChangeShowTime, this);
	OnChangeShowTime();

	SetTimeStep(true);
    _gameContext.GetGameEventSource().AddListener(*this);
}
    
UI::GameLayout::~GameLayout()
{
	_gameContext.GetGameEventSource().RemoveListener(*this);
	g_conf.ui_showtime.eventChange = nullptr;
}


void UI::GameLayout::OnTimeStep(float dt)
{
	bool tab = GetManager().GetInput().IsKeyPressed(GLFW_KEY_TAB);
	_score->SetVisible(tab || _gameContext.GetGameplay().IsGameOver());

    _gameViewHarness.Step(dt);
	
	bool readUserInput = !GetManager().GetFocusWnd() || this == GetManager().GetFocusWnd();
	WorldController::ControllerStateMap controlStates;
    
    if (readUserInput)
    {
        FOREACH( _gameContext.GetWorld().GetList(LIST_players), GC_Player, player )
        {
            if( GC_Vehicle *vehicle = player->GetVehicle() )
            {
                if( Controller *controller = _inputMgr.GetController(player) )
                {
                    vec2d mouse = GetManager().GetInput().GetMousePos();
                    auto c2w = _gameViewHarness.CanvasToWorld(*player, (int) mouse.x, (int) mouse.y);

                    VehicleState vs;
                    controller->ReadControllerState(GetManager().GetInput(), _gameContext.GetWorld(),
                                                    vehicle, c2w.visible ? &c2w.worldPos : nullptr, vs);
                    controlStates.insert(std::make_pair(vehicle->GetId(), vs));
                }
            }
        }
	
		_worldController.SendControllerStates(std::move(controlStates));
    }
}

void UI::GameLayout::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
    vec2d eye(_defaultCamera.GetPos().x + GetWidth() / 2, _defaultCamera.GetPos().y + GetHeight() / 2);
    float zoom = _defaultCamera.GetZoom();
    _gameViewHarness.RenderGame(dc, _worldView, eye, zoom);
	dc.SetMode(RM_INTERFACE);
	Window::DrawChildren(dc, sx, sy);
}
    
void UI::GameLayout::OnSize(float width, float height)
{
	_time->Move(GetWidth() - 1, GetHeight() - 1);
	_msg->Move(_msg->GetX(), GetHeight() - 50);
    _gameViewHarness.SetCanvasSize((int) GetWidth(), (int) GetHeight());
}

void UI::GameLayout::OnChangeShowTime()
{
	_time->SetVisible(g_conf.ui_showtime.Get());
}

void UI::GameLayout::OnGameMessage(const char *msg)
{
    _msg->WriteLine(msg);
}


