// gui_editor.h

#pragma once

#include "WorldEvents.h"
#include "core/SafePtr.h"

#include <Dialog.h>
#include <List.h>

// forward declarations
class World;
class GC_Object;
class PropertySet;

namespace UI
{
template <class, class> class ListAdapter;
class ComboBox;
class Button;
class Edit;
class Text;

class NewMapDlg : public Dialog
{
	Edit *_width;
	Edit *_height;

public:
	NewMapDlg(Window *parent);

	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////

class PropertyList : public Dialog
{
	class Container : public Window
	{
//		bool OnRawChar(int c); // need to pass messages through
	public:
		Container(Window *parent);
	};


	Container  *_psheet;
	ScrollBarVertical  *_scrollBar;

	SafePtr<PropertySet>  _ps;
	std::vector<Window*>  _ctrls;
    World &_world;

public:
	PropertyList(Window *parent, float x, float y, float w, float h, World &world);
	void ConnectTo(const SafePtr<PropertySet> &ps);
	void DoExchange(bool applyToObject);

protected:
	void OnScroll(float pos);
	void OnSize(float width, float height);
	bool OnRawChar(int c);
	bool OnMouseWheel(float x, float y, float z);
};

///////////////////////////////////////////////////////////////////////////////

class ServiceListDataSource
	: public ListDataSource
	, public ObjectListener
{
public:
	// ListDataSource implementation
	virtual void AddListener(ListDataSourceListener *listener);
	virtual void RemoveListener(ListDataSourceListener *listener);
	virtual int GetItemCount() const;
	virtual int GetSubItemCount(int index) const;
	virtual size_t GetItemData(int index) const;
	virtual const std::string& GetItemText(int index, int sub) const;
	virtual int FindItem(const std::string &text) const;

	// ObjectListener implementation
	virtual void OnCreate(GC_Object *obj);
	virtual void OnKill(GC_Object *obj);

public:
	ServiceListDataSource(World &world);
	~ServiceListDataSource();

private:
	mutable std::string _nameCache;
	ListDataSourceListener *_listener;
    World &_world;
};

// forward declaration
class EditorLayout;

class ServiceEditor : public Dialog
{
	typedef ListAdapter<ListDataSourceDefault, ComboBox> DefaultComboBox;

    ServiceListDataSource _listData;
	List *_list;
	DefaultComboBox *_combo;
	Text *_labelService;
	Text *_labelName;
	Button *_btnCreate;

	float _margins;
    World &_world;

public:
	ServiceEditor(Window *parent, float x, float y, float w, float h, World &world);
	virtual ~ServiceEditor();

protected:
	void OnChangeSelectionGlobal(GC_Object *obj);

	void OnCreateService();
	void OnSelectService(int i);
	EditorLayout* GetEditorLayout() const;

	virtual void OnSize(float width, float height);
	virtual bool OnRawChar(int c);
};

///////////////////////////////////////////////////////////////////////////////

class EditorLayout : public Window
{
	typedef ListAdapter<ListDataSourceDefault, ComboBox> DefaultComboBox;

	PropertyList *_propList;
	ServiceEditor    *_serviceList;
	Text         *_layerDisp;
	DefaultComboBox  *_typeList;
	Text         *_help;
	size_t        _fontSmall;

	size_t       _selectionRect;

	GC_Object *_selectedObject;
	bool _isObjectNew;
	bool _click;
	int  _mbutton;
    World &_world;


	void OnKillSelected(World &world, GC_Object *sender, void *param);
	void OnMoveSelected(World &world, GC_Object *sender, void *param);

public:
	EditorLayout(Window *parent, World &world);
	virtual ~EditorLayout();

	void Select(GC_Object *object, bool bSelect);
	void SelectNone();

	std::function<void(GC_Object*)> eventOnChangeSelection;

protected:
	void DrawChildren(DrawingContext &dc, float sx, float sy) const;

	bool OnMouseWheel(float x, float y, float z);
	bool OnMouseDown(float x, float y, int button);
	bool OnMouseUp(float x, float y, int button);
	bool OnMouseMove(float x, float y);
	bool OnFocus(bool focus);
	bool OnRawChar(int c);
	void OnSize(float width, float height);
	void OnVisibleChange(bool visible, bool inherited);

	void OnChangeObjectType(int index);
	void OnChangeUseLayers();
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
