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
}

void MouseCursor::Draw(float sx, float sy)
{
	Move( (float) g_env.envInputs.mouse_x, (float) g_env.envInputs.mouse_y );
	Window::Draw(sx, sy);
}

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

