// MousePointer.cpp

#include "MousePointer.h"
#include "Text.h"

#include "Level.h"
#include "gc/Camera.h"

#include <GLFW/glfw3.h>

namespace UI
{

MouseCursor::MouseCursor(LayoutManager* manager, const char *texture)
  : Window(NULL, manager)
{
	SetTexture(texture, true);
	_text = Text::Create(this, GetWidth(), GetHeight(), "", alignTextLT);
	_timeShow = 0;
	_timeAnim = 0;

	SetTimeStep(true);
}

void MouseCursor::OnTimeStep(float dt)
{
	_timeAnim += dt;
	_timeShow += dt;

    double mouse_x = 0, mouse_y = 0;
    glfwGetCursorPos(g_appWindow, &mouse_x, &mouse_y);

	int newx = (int) mouse_x;
	int newy = (int) mouse_y;

	if( int(GetX()) != newx || int(GetY()) != newy )
	{
		_timeShow = 0;
		Move((float) newx, (float) newy);
	}

//	if( g_env.nNeedCursor || _timeShow < 1 )
//		SetOpacity1i(255);
//	else
//		SetOpacity1i(255 - std::min(255, int((_timeShow - 1) * 255)));

	SetVisible( g_env.nNeedCursor > 0 || _timeShow < 2 );

	vec2d ptCurPos;
	if( GetVisible() &&
    //    g_level->GetEditorMode() &&
        GC_Camera::GetWorldMousePos(ptCurPos, true) )
	{
		char str[32];
		sprintf(str, "%d,%d", int(ptCurPos.x) / CELL_SIZE + 1, int(ptCurPos.y) / CELL_SIZE + 1);
		_text->SetText(str);
		_text->SetVisible(true);
	}
	else
	{
		_text->SetVisible(false); // hide text if coordinates is not available
	}

	if( GetVisible() )
		SetFrame( abs((int) GetFrameCount()-1 - int((_timeAnim * ANIMATION_FPS / 3)) % ((int) GetFrameCount() * 2 - 2)) );

}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

