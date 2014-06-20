#include "gui_game.h"
#include "gui_messagearea.h"
#include "gui_scoretable.h"
#include "DefaultCamera.h"
#include "globals.h"
#include "InputManager.h"
#include "Controller.h"
#include "Macros.h"
#include "WorldController.h"
#include "config/Config.h"
#include "gc/World.h"
#include "gc/Camera.h"
#include "gc/Player.h"
#include "gc/Vehicle.h"
#include "render/WorldView.h"

#include <GuiManager.h>
    

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
	if( !_world.IsEmpty() )
	{
		char text[16];
		int time = (int) _world.GetTime();

		if( time % 60 < 10 )
			sprintf(text, "%d:0%d", time / 60, time % 60);
		else
			sprintf(text, "%d:%d", time / 60, time % 60);

		SetText(text);
	}
	else
	{
		SetText("--:--");
	}
}

///////////////////////////////////////////////////////////////////////////////

UI::GameLayout::GameLayout(Window *parent,
						   World &world,
						   WorldView &worldView,
						   WorldController &worldController,
						   InputManager &inputMgr,
						   const DefaultCamera &defaultCamera)
    : Window(parent)
    , _world(world)
    , _worldView(worldView)
	, _worldController(worldController)
	, _inputMgr(inputMgr)
    , _defaultCamera(defaultCamera)
{
	_msg = new MessageArea(this, 100, 100);
    
	_score = new ScoreTable(this, _world);
	_score->SetVisible(false);
    
	_time = new TimeElapsed(this, 0, 0, alignTextRB, _world);
	g_conf.ui_showtime.eventChange = std::bind(&GameLayout::OnChangeShowTime, this);
	OnChangeShowTime();

    assert(!world._messageListener);
    world._messageListener = this;
	
	SetTimeStep(true);
}
    
UI::GameLayout::~GameLayout()
{
    assert(this == _world._messageListener);
    _world._messageListener = nullptr;
	g_conf.ui_showtime.eventChange = nullptr;
}


static Rect GetCameraViewport(Point ssize, Point wsize, size_t camCount, size_t camIndex)
{
    assert(camCount > 0 && camCount <= 4);
    assert(camIndex < camCount);
    
	if( ssize.x >= wsize.x && ssize.y >= wsize.y )
	{
		return CRect((ssize.x - wsize.x) / 2, (ssize.y - wsize.y) / 2,
                     (ssize.x + wsize.x) / 2, (ssize.y + wsize.y) / 2);
	}

    Rect viewports[4];

    switch( camCount )
    {
        case 1:
            viewports[0] = CRect(0,             0,             ssize.x,       ssize.y);
            break;
        case 2:
            viewports[0] = CRect(0,             0,             ssize.x/2 - 1, ssize.y);
            viewports[1] = CRect(ssize.x/2 + 1, 0,             ssize.x,       ssize.y);
            break;
        case 3:
            viewports[0] = CRect(0,             0,             ssize.x/2 - 1, ssize.y/2 - 1);
            viewports[1] = CRect(ssize.x/2 + 1, 0,             ssize.x,       ssize.y/2 - 1);
            viewports[2] = CRect(ssize.x/4,     ssize.y/2 + 1, ssize.x*3/4,   ssize.y);
            break;
        case 4:
            viewports[0] = CRect(0,             0,             ssize.x/2 - 1, ssize.y/2 - 1);
            viewports[1] = CRect(ssize.x/2 + 1, 0,             ssize.x,       ssize.y/2 - 1);
            viewports[2] = CRect(0,             ssize.y/2 + 1, ssize.x/2 - 1, ssize.y);
            viewports[3] = CRect(ssize.x/2 + 1, ssize.y/2 + 1, ssize.x,       ssize.y);
            break;
        default:
            assert(false);
    }
    
    return viewports[camIndex];
}

void UI::GameLayout::OnTimeStep(float dt)
{
	bool readUserInput = !GetManager()->GetFocusWnd() || this == GetManager()->GetFocusWnd();
	WorldController::ControllerStateMap controlStates;
	
	size_t camIndex = 0;
	size_t camCount = _world.GetList(LIST_cameras).size();
	FOREACH( _world.GetList(LIST_cameras), GC_Camera, pCamera )
	{
		Rect screen = GetCameraViewport(Point{(int) GetWidth(), (int) GetHeight()},
										Point{(int) _world._sx, (int) _world._sy}, camCount, camIndex);
		pCamera->CameraTimeStep(_world, dt, vec2d((float) WIDTH(screen), (float) HEIGHT(screen)));
		if (readUserInput)
		{
			GC_Player *player = pCamera->GetPlayer();
			if( GC_Vehicle *vehicle = player->GetVehicle() )
			{
				bool mouseInViewport = false;
				vec2d pt = GetManager()->GetMousePos();
				if( PtInRect(screen, Point{(int) pt.x, (int) pt.y}) )
				{
					FRECT w;
					pCamera->GetWorld(w, screen);
					pt.x = w.left + (pt.x - (float) screen.left) / pCamera->GetZoom();
					pt.y = w.top + (pt.y - (float) screen.top) / pCamera->GetZoom();
					mouseInViewport = true;
				}
				if( Controller *controller = _inputMgr.GetController(player) )
				{
					VehicleState vs;
					controller->ReadControllerState(_world, vehicle, mouseInViewport ? &pt : nullptr, vs);
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
            
            Rect screen = GetCameraViewport(Point{(int) GetWidth(), (int) GetHeight()},
                                            Point{(int) _world._sx, (int) _world._sy}, camCount, 0);

            FRECT viewRect;
            singleCamera->GetWorld(viewRect, screen);
            
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
			size_t camIndex = 0;
            FOREACH( _world.GetList(LIST_cameras), GC_Camera, pCamera )
            {
				Rect screen = GetCameraViewport(Point{(int) GetWidth(), (int) GetHeight()},
												Point{(int) _world._sx, (int) _world._sy}, camCount, camIndex);

				FRECT viewRect;
                pCamera->GetWorld(viewRect, screen);

                g_render->Camera(&screen,
                                 viewRect.left,
                                 viewRect.top,
                                 pCamera->GetZoom(),
                                 g_conf.g_rotcamera.Get() ? pCamera->GetAngle() : 0);
                
                _worldView.Render(_world, viewRect, false);
				
				++camIndex;
            }
        }
    }
    else
    {
        // render from default camera
		FRECT viewRect;
		viewRect.left = _defaultCamera.GetPosX();
		viewRect.top = _defaultCamera.GetPosY();
		viewRect.right = viewRect.left + (float) GetWidth() / _defaultCamera.GetZoom();
		viewRect.bottom = viewRect.top + (float) GetHeight() / _defaultCamera.GetZoom();
        
		g_render->Camera(NULL, _defaultCamera.GetPosX(), _defaultCamera.GetPosY(), _defaultCamera.GetZoom(), 0);
        _worldView.Render(_world, viewRect, false);
    }
	g_render->SetMode(RM_INTERFACE);
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


