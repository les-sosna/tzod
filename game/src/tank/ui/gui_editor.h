// gui_editor.h

#pragma once

#include "ui/Base.h"
#include "ui/Dialog.h"

// forward declarations
class GC_Object;
class PropertySet;

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class PropertyList : public Dialog
{
	Window     *_psheet;
	ScrollBar  *_scrollBar;

	SafePtr<PropertySet> _ps;
	std::vector<Window*>  _ctrls;


public:
	PropertyList(Window *parent, float x, float y, float w, float h);

	void ConnectTo(const SafePtr<PropertySet> &ps);

protected:
	void OnScroll(float pos);
	void OnSize(float width, float height);
	void OnRawChar(int c);

};

///////////////////////////////////////////////////////////////////////////////

class EditorLayout : public Window
{
	PropertyList *_proplist;

	GC_Object *_selectedObject;
	void OnKillSelected(GC_Object *sender, void *param);
	void OnMoveSelected(GC_Object *sender, void *param);
	void Select(GC_Object *object, bool bSelect);

public:
	EditorLayout(Window *parent);

protected:
	bool OnMouseDown(float x, float y, int button);
	bool OnFocus(bool focus);
	void OnRawChar(int c);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
