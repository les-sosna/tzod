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

class NewMapDlg : public Dialog
{
	Edit *_width;
	Edit *_height;

public:
	NewMapDlg(Window *parent);
	~NewMapDlg();

	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////

class MapSettingsDlg : public Dialog
{
public:
	MapSettingsDlg(Window *parent);
	~MapSettingsDlg();
};

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

public:
	PropertyList(Window *parent, float x, float y, float w, float h);
	void ConnectTo(const SafePtr<PropertySet> &ps);
	void Exchange(bool applyToObject);

protected:
	void OnScroll(float pos);
	void OnSize(float width, float height);
	void OnRawChar(int c);
	bool OnMouseWheel(float x, float y, float z);
};

///////////////////////////////////////////////////////////////////////////////

class EditorLayout : public Window
{
	PropertyList *_propList;
	ComboBox     *_typeList;
	Window       *_selectionRect;

	GC_Object *_selectedObject;
	bool _isObjectNew;

	void OnKillSelected(GC_Object *sender, void *param);
	void OnMoveSelected(GC_Object *sender, void *param);
	void Select(GC_Object *object, bool bSelect);

public:
	EditorLayout(Window *parent);

protected:
	void DrawChildren(float sx, float sy);

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
