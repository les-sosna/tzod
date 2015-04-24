#include "gui_game.h"
#include "gui_messagearea.h"
#include "gui_scoretable.h"
#include "DefaultCamera.h"
#include "InputManager.h"
#include "Controller.h"
#include "Deathmatch.h"
#include "WorldController.h"
#include "config/Config.h"
#include <gc/World.h>
#include <gc/Camera.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/Macros.h>
#include "render/WorldView.h"

#include <GLFW/glfw3.h>
#include <ui/GuiManager.h>
#include <ui/UIInput.h>
#include <video/DrawingContext.h>

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
						   InputManager &inputMgr,
						   Gameplay &gameplay,
						   const DefaultCamera &defaultCamera)
    : Window(parent)
	, _gameEventSource(gameEventSource)
    , _world(world)
    , _worldView(worldView)
	, _worldController(worldController)
	, _inputMgr(inputMgr)
	, _gameplay(gameplay)
    , _defaultCamera(defaultCamera)
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


static RectRB GetCameraViewport(int screenW, int screenH, size_t camCount, size_t camIndex)
{
    assert(camCount > 0 && camCount <= 4);
    assert(camIndex < camCount);
    
    RectRB viewports[4];

    switch( camCount )
    {
        case 1:
            viewports[0] = CRect(0,             0,             screenW,       screenH);
            break;
        case 2:
            viewports[0] = CRect(0,             0,             screenW/2 - 1, screenH);
            viewports[1] = CRect(screenW/2 + 1, 0,             screenW,       screenH);
            break;
        case 3:
            viewports[0] = CRect(0,             0,             screenW/2 - 1, screenH/2 - 1);
            viewports[1] = CRect(screenW/2 + 1, 0,             screenW,       screenH/2 - 1);
            viewports[2] = CRect(screenW/4,     screenH/2 + 1, screenW*3/4,   screenH);
            break;
        case 4:
            viewports[0] = CRect(0,             0,             screenW/2 - 1, screenH/2 - 1);
            viewports[1] = CRect(screenW/2 + 1, 0,             screenW,       screenH/2 - 1);
            viewports[2] = CRect(0,             screenH/2 + 1, screenW/2 - 1, screenH);
            viewports[3] = CRect(screenW/2 + 1, screenH/2 + 1, screenW,       screenH);
            break;
        default:
            assert(false);
    }
    
    return viewports[camIndex];
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
	if( size_t camCount = _world.GetList(LIST_cameras).size() )
	{
        int width = (int) GetWidth();
		int height = (int) GetHeight();
		
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

			RectRB viewport = CRect((width - (int) _world._sx) / 2, (height - (int) _world._sy) / 2,
			                        (width + (int) _world._sx) / 2, (height + (int) _world._sy) / 2);
			vec2d eye = singleCamera->GetCameraPos();
			float zoom = singleCamera->GetZoom();
			_worldView.Render(dc, _world, viewport, eye, zoom, false, false, _world.GetNightMode());
		}
		else
		{
			// render from each camera
			size_t camIndex = 0;
			FOREACH( _world.GetList(LIST_cameras), GC_Camera, pCamera )
			{
				RectRB viewport = GetCameraViewport(width, height, camCount, camIndex);
				vec2d eye = pCamera->GetCameraPos();
				float zoom = pCamera->GetZoom();
				_worldView.Render(dc, _world, viewport, eye, zoom, false, false, _world.GetNightMode());
				++camIndex;
			}
		}
	}
	else
	{
		// render from default camera
		CRect viewport(0, 0, (int) GetWidth(), (int) GetHeight());
		vec2d eye(_defaultCamera.GetPos().x + GetWidth() / 2, _defaultCamera.GetPos().y + GetHeight() / 2);
		float zoom = _defaultCamera.GetZoom();
		_worldView.Render(dc, _world, viewport, eye, zoom, false, false, _world.GetNightMode());
	}
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


