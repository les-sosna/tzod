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
	class Container : public Window
	{
		void OnRawChar(int c); // need to pass messages through
	public:
		Container(Window *parent);
	};


	Container  *_psheet;
	ScrollBar  *_scrollBar;

	SafePtr<PropertySet>  _ps;
	std::vector<Window*>  _ctrls;

	void Commit();

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
	PropertyList *_propList;
	ComboBox     *_typeList;

	GC_Object *_selectedObject;
	void OnKillSelected(GC_Object *sender, void *param);
	void OnMoveSelected(GC_Object *sender, void *param);
	void Select(GC_Object *object, bool bSelect);

public:
	EditorLayout(Window *parent);

protected:
	bool OnMouseWheel(float x, float y, float z);
	bool OnMouseDown(float x, float y, int button);
	bool OnFocus(bool focus);
	void OnRawChar(int c);
	void OnSize(float width, float height);
	void OnShow(bool show);

	void OnChangeObject(int index);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
