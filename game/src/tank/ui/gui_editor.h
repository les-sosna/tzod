// gui_editor.h

#pragma once

#include "ui/Base.h"
#include "ui/Window.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class MyPropertySheet : public Window
{
public:
	MyPropertySheet(Window *parent);
};

///////////////////////////////////////////////////////////////////////////////

class PropertyList : public Window
{
	MyPropertySheet *_psheet;
	ScrollBar *_scrollBar;

public:
	PropertyList(Window *parent, float x, float y, float w, float h);


protected:
	void OnScroll(float pos);

};

///////////////////////////////////////////////////////////////////////////////

class EditorLayout : public Window
{
	PropertyList *_proplist;

public:
	EditorLayout(Window *parent);

};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
