// MousePointer.cpp

#include "stdafx.h"
#include "MousePointer.h"
#include "Text.h"

namespace UI
{
;

///////////////////////////////////////////////////////////////////////////////
// MouseCursor class implementation

MouseCursor::MouseCursor(GuiManager* manager, const char *texture)
  : Window(manager)
{
	SetTexture(texture);
	Resize(GetTextureWidth(), GetTextureHeight());
	_text = new Text(this, GetWidth(), GetHeight(), "", alignTextLT);
	_timeShow = 0;
	_timeAnim = 0;

	SetTimeStep(true);
}

void MouseCursor::Draw(float sx, float sy)
{
//	Move( (float) g_env.envInputs.mouse_x, (float) g_env.envInputs.mouse_y );
	Window::Draw(sx, sy);
}

void MouseCursor::OnTimeStep(float dt)
{
	_timeAnim += dt;
	_timeShow += dt;

	int newx = g_env.envInputs.mouse_x;
	int newy = g_env.envInputs.mouse_y;

	if( int(GetX()) != newx || int(GetY()) != newy )
	{
		_timeShow = 0;
		Move((float) newx, (float) newy);
	}

//	if( g_env.nNeedCursor || _timeShow < 1 )
//		SetOpacity1i(255);
//	else
//		SetOpacity1i(255 - __min(255, int((_timeShow - 1) * 255)));

	Show( g_env.nNeedCursor > 0 || _timeShow < 2 );

//	vec2d ptCurPos;
//	if( IsVisible() && GC_Camera::GetWorldMousePos(ptCurPos) )
//	{
//		char str[32];
//		sprintf(str, "%d,%d", int(ptCurPos.x) / CELL_SIZE + 1, int(ptCurPos.y) / CELL_SIZE + 1);
//		_text->SetText(str);
//		_text->Show(true);
//	}
//	else
//	{
//		_text->Show(false);
//	}

	if( IsVisible() )
		SetFrame( abs(GetFrameCount()-1 - int((_timeAnim * ANIMATION_FPS / 3)) % (GetFrameCount() * 2-2)) );

}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

