#include "Config.h"
#include "Controller.h"
#include "DefaultCamera.h"
#include "GameView.h"
#include "gui_game.h"
#include "gui_messagearea.h"
#include "gui_scoretable.h"
#include "InputManager.h"

#include <app/Deathmatch.h>
#include <app/WorldController.h>
#include <gc/World.h>
#include <gc/Camera.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/Macros.h>
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
						   GameEventSource &gameEventSource,
						   World &world,
						   WorldView &worldView,
						   WorldController &worldController,
						   Gameplay &gameplay,
						   const DefaultCamera &defaultCamera)
    : Window(parent)
	, _gameEventSource(gameEventSource)
    , _world(world)
    , _worldView(worldView)
	, _worldController(worldController)
	, _gameplay(gameplay)
    , _defaultCamera(defaultCamera)
    , _inputMgr(world)
{
	_msg = new MessageArea(this, 100, 100);
    
	_score = new ScoreTable(this, _world, dynamic_cast<Deathmatch&>(gameplay));
	_score->SetVisible(false);
    
	_time = new TimeElapsed(this, 0, 0, alignTextRB, _world);
	g_conf.ui_showtime.eventChange = std::bind(&GameLayout::OnChangeShowTime, this);
	OnChangeShowTime();

	SetTimeStep(true);
	_gameEventSource.AddListener(*this);
}
    
UI::GameLayout::~GameLayout()
{
	_gameEventSource.RemoveListener(*this);
	g_conf.ui_showtime.eventChange = nullptr;
}


void UI::GameLayout::OnTimeStep(float dt)
{
	bool tab = GetManager().GetInput().IsKeyPressed(GLFW_KEY_TAB);
	_score->SetVisible(tab || _gameplay.IsGameOver());

	
	bool readUserInput = !GetManager().GetFocusWnd() || this == GetManager().GetFocusWnd();
	WorldController::ControllerStateMap controlStates;
	
	size_t camIndex = 0;
	size_t camCount = _world.GetList(LIST_cameras).size();
	FOREACH( _world.GetList(LIST_cameras), GC_Camera, pCamera )
	{
		RectRB viewport = GetCameraViewport((int) GetWidth(), (int) GetHeight(), camCount, camIndex);
		pCamera->CameraTimeStep(_world, dt, vec2d((float) WIDTH(viewport), (float) HEIGHT(viewport)) / pCamera->GetZoom());
		if (readUserInput)
		{
			GC_Player *player = pCamera->GetPlayer();
			if( GC_Vehicle *vehicle = player->GetVehicle() )
			{
				bool mouseInViewport = false;
				vec2d ptWorld(0,0);
				vec2d ptScreen = GetManager().GetInput().GetMousePos();
				if( PtInRect(viewport, (int) ptScreen.x, (int) ptScreen.y) )
				{
					vec2d eye = pCamera->GetCameraPos();
					float zoom = pCamera->GetZoom();
					ptWorld.x = eye.x + (ptScreen.x - (viewport.left + viewport.right) / 2) / zoom;
					ptWorld.y = eye.y + (ptScreen.y - (viewport.bottom + viewport.top) / 2) / zoom;
					mouseInViewport = true;
				}
				if( Controller *controller = _inputMgr.GetController(player) )
				{
					VehicleState vs;
					controller->ReadControllerState(GetManager().GetInput(), _world, vehicle, mouseInViewport ? &ptWorld : nullptr, vs);
					controlStates.insert(std::make_pair(vehicle->GetId(), vs));
				}
			}
		}
		++camIndex;
	}
	
	if (readUserInput)
		_worldController.SendControllerStates(std::move(controlStates));
}

void UI::GameLayout::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
    RenderGame(dc, _world, _worldView, (int) GetWidth(), (int) GetHeight(), _defaultCamera);
	dc.SetMode(RM_INTERFACE);
	Window::DrawChildren(dc, sx, sy);
}
    
void UI::GameLayout::OnSize(float width, float height)
{
	_time->Move(GetWidth() - 1, GetHeight() - 1);
	_msg->Move(_msg->GetX(), GetHeight() - 50);
}

void UI::GameLayout::OnChangeShowTime()
{
	_time->SetVisible(g_conf.ui_showtime.Get());
}

void UI::GameLayout::OnGameMessage(const char *msg)
{
    _msg->WriteLine(msg);
}


