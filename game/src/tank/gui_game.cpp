#include "gui_game.h"
#include "gui_messagearea.h"
#include "gui_scoretable.h"
#include "DefaultCamera.h"
#include "globals.h"
#include "Macros.h"
#include "config/Config.h"
#include "gc/World.h"
#include "gc/Camera.h"
#include "render/WorldView.h"
    

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

UI::GameLayout::GameLayout(Window *parent, World &world, WorldView &worldView, const DefaultCamera &defaultCamera)
    : Window(parent)
    , _world(world)
    , _worldView(worldView)
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
}
    
UI::GameLayout::~GameLayout()
{
    assert(this == _world._messageListener);
    _world._messageListener = nullptr;
	g_conf.ui_showtime.eventChange = nullptr;
}
    
void UI::GameLayout::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
    if( _world.GetList(LIST_cameras).empty() )
    {
		FRECT viewRect;
		viewRect.left = _defaultCamera.GetPosX();
		viewRect.top = _defaultCamera.GetPosY();
		viewRect.right = viewRect.left + (float) GetWidth() / _defaultCamera.GetZoom();
		viewRect.bottom = viewRect.top + (float) GetHeight() / _defaultCamera.GetZoom();
        
		g_render->Camera(NULL, _defaultCamera.GetPosX(), _defaultCamera.GetPosY(), _defaultCamera.GetZoom(), 0);
        _worldView.Render(_world, viewRect, false);
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
    
void UI::GameLayout::OnSize(float width, float height)
{
	_time->Move( GetWidth() - 1, GetHeight() - 1 );
	_msg->Move(_msg->GetX(), GetHeight() - 50);
    GC_Camera::UpdateLayout(_world, width, height);
}

void UI::GameLayout::OnChangeShowTime()
{
	_time->SetVisible(g_conf.ui_showtime.Get());
}

void UI::GameLayout::OnGameMessage(const char *msg)
{
    _msg->WriteLine(msg);
}


