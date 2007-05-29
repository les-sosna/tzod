// gui_editor.cpp

#include "stdafx.h"

#include "gui_editor.h"

#include "ui/Text.h"
#include "ui/Scroll.h"



namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// PropertySheet class implementation

MyPropertySheet::MyPropertySheet(Window *parent) : Window(parent)
{
	new Text(this, 20, 30, "hello", alignTextLT);
}


///////////////////////////////////////////////////////////////////////////////
// PropertyList class implementation

PropertyList::PropertyList(Window *parent, float x, float y, float w, float h)
  : Window(parent, x, y, "window")
{
	_psheet = new MyPropertySheet(this);

	_scrollBar = new ScrollBar(this, 0, 0, h);
	_scrollBar->Move(w - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll.bind(&PropertyList::OnScroll, this);
	_scrollBar->SetLimit(100);

	Resize(w, h);
	SetBorder(true);
	ClipChildren(true);
}

void PropertyList::OnScroll(float pos)
{
	_psheet->Move(0, -floorf(_scrollBar->GetPos()));
}

///////////////////////////////////////////////////////////////////////////////

EditorLayout::EditorLayout(Window *parent) : Window(parent)
{
	SetTexture(NULL);
	_proplist = new PropertyList(this, 10, 10, 256, 512);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
