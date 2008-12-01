// gui_editor.h

#pragma once

#include "ui/Base.h"
#include "ui/Dialog.h"
#include "ui/List.h"
#include "ObjectListener.h"

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
	void DoExchange(bool applyToObject);

protected:
	void OnScroll(float pos);
	void OnSize(float width, float height);
	void OnRawChar(int c);
	bool OnMouseWheel(float x, float y, float z);
};

///////////////////////////////////////////////////////////////////////////////

class ServiceListDataSource
	: public ListDataSource
	, public ObjectListener
{
public:
	virtual void SetListener(ListDataSourceListener *listener);

	virtual int GetItemCount() const;
	virtual int GetSubItemCount(int index) const;
	virtual ULONG_PTR GetItemData(int index) const;
	virtual const string_t& GetItemText(int index, int sub) const;
	virtual int FindItem(const string_t &text) const;

	// ObjectListener implementation
	virtual void OnCreate(GC_Object *obj);
	virtual void OnKill(GC_Object *obj);

public:
	ServiceListDataSource();
	virtual ~ServiceListDataSource();

private:
	mutable string_t _nameCache;
//	std::set<GC_Object*> _cache;
	ListDataSourceListener *_listener;
};


class EditorLayout;

class ServiceList : public Dialog
{
	List *_list;
	Text *_labelService;
	Text *_labelName;
	ComboBox *_combo;
	Button *_btnCreate;

	float _margins;

public:
	ServiceList(Window *parent, float x, float y, float w, float h);
	virtual ~ServiceList();

protected:
	void OnChangeSelectionGlobal(GC_Object *obj);

	void OnCreateService();
	void OnSelectService(int i);
	EditorLayout* GetEditorLayout() const;

	virtual void OnSize(float width, float height);
	virtual void OnRawChar(int c);
};

///////////////////////////////////////////////////////////////////////////////

class EditorLayout : public Window
{
	PropertyList *_propList;
	ServiceList  *_serviceList;
	Text         *_layerDisp;
	ComboBox     *_typeList;
	Window       *_selectionRect;
	Text         *_help;

	GC_Object *_selectedObject;
	bool _isObjectNew;
	bool _click;
	int  _mbutton;


	void OnKillSelected(GC_Object *sender, void *param);
	void OnMoveSelected(GC_Object *sender, void *param);

public:
	EditorLayout(Window *parent);
	~EditorLayout();

	void Select(GC_Object *object, bool bSelect);
	void SelectNone();

	Delegate<void(GC_Object*)> eventOnChangeSelection;

protected:
	void DrawChildren(float sx, float sy);

	bool OnMouseWheel(float x, float y, float z);
	bool OnMouseDown(float x, float y, int button);
	bool OnMouseUp(float x, float y, int button);
	bool OnMouseMove(float x, float y);
	bool OnFocus(bool focus);
	void OnRawChar(int c);
	void OnSize(float width, float height);
	void OnShow(bool show);

	void OnChangeObjectType(int index);
	void OnChangeUseLayers();
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
