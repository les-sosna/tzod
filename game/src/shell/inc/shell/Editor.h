#pragma once
#include <gc/WorldEvents.h>
#include <ui/Dialog.h>
#include <ui/List.h>
#include <memory>

class ConfCache;
class World;
class WorldView;
class GC_Object;
class PropertySet;
class DefaultCamera;
class ThemeManager;
struct lua_State;

namespace UI
{
	template <class, class> class ListAdapter;
	class ComboBox;
	class ConsoleBuffer;
	class Button;
	class Edit;
	class Text;
}

class NewMapDlg : public UI::Dialog
{
	ConfCache &_conf;
	UI::Edit *_width;
	UI::Edit *_height;

public:
	NewMapDlg(Window *parent, ConfCache &conf);

	void OnOK();
	void OnCancel();
};

///////////////////////////////////////////////////////////////////////////////

class PropertyList : public UI::Dialog
{
	class Container : public UI::Window
	{
//		bool OnRawChar(int c); // need to pass messages through
	public:
		Container(UI::Window *parent);
	};

	Container  *_psheet;
	UI::ScrollBarVertical  *_scrollBar;

	std::shared_ptr<PropertySet>  _ps;
	std::vector<Window*>  _ctrls;
	World &_world;
	ConfCache &_conf;
	UI::ConsoleBuffer &_logger;

public:
	PropertyList(Window *parent, float x, float y, float w, float h, World &world, ConfCache &_conf, UI::ConsoleBuffer &logger);
	void ConnectTo(std::shared_ptr<PropertySet> ps);
	void DoExchange(bool applyToObject);

protected:
	void OnScroll(float pos);
	void OnSize(float width, float height);
	bool OnRawChar(int c);
	bool OnMouseWheel(float x, float y, float z);
};

///////////////////////////////////////////////////////////////////////////////

class ServiceListDataSource
	: public UI::ListDataSource
	, public ObjectListener<World>
{
public:
	// ListDataSource implementation
	virtual void AddListener(UI::ListDataSourceListener *listener);
	virtual void RemoveListener(UI::ListDataSourceListener *listener);
	virtual int GetItemCount() const;
	virtual int GetSubItemCount(int index) const;
	virtual size_t GetItemData(int index) const;
	virtual const std::string& GetItemText(int index, int sub) const;
	virtual int FindItem(const std::string &text) const;

	// ObjectListener<World>
	virtual void OnGameStarted() override {}
	virtual void OnGameFinished() override {}
	virtual void OnNewObject(GC_Object &obj) override;
	virtual void OnKill(GC_Object &obj) override;

public:
	ServiceListDataSource(World &world);
	~ServiceListDataSource();

private:
	mutable std::string _nameCache;
	UI::ListDataSourceListener *_listener;
    World &_world;
};

// forward declaration
class EditorLayout;

class ServiceEditor : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

    ServiceListDataSource _listData;
	UI::List *_list;
	DefaultComboBox *_combo;
	UI::Text *_labelService;
	UI::Text *_labelName;
	UI::Button *_btnCreate;

	float _margins;
	World &_world;
	ConfCache &_conf;

public:
	ServiceEditor(UI::Window *parent, float x, float y, float w, float h, World &world, ConfCache &conf);
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

class ConfCache;

class EditorLayout : public UI::Window
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	ConfCache &_conf;
	UI::ConsoleBuffer &_logger;
	const DefaultCamera &_defaultCamera;
	PropertyList *_propList;
	ServiceEditor    *_serviceList;
	UI::Text         *_layerDisp;
	DefaultComboBox  *_typeList;
	UI::Text         *_help;
	size_t        _fontSmall;

	size_t       _selectionRect;

	GC_Object *_selectedObject;
	bool _isObjectNew;
	bool _click;
	int  _mbutton;
    World &_world;
    WorldView &_worldView;
	lua_State *_globL;


	void OnKillSelected(World &world, GC_Object *sender, void *param);
	void OnMoveSelected(World &world, GC_Object *sender, void *param);

public:
	EditorLayout(UI::Window *parent, World &world, WorldView &worldView, const DefaultCamera &defaultCamera, lua_State *globL, ConfCache &conf, UI::ConsoleBuffer &logger);
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

class MapSettingsDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;
	DefaultComboBox *_theme;
	UI::Edit *_author;
	UI::Edit *_email;
	UI::Edit *_url;
	UI::Edit *_desc;
	UI::Edit *_onInit;
    World &_world;

public:
	MapSettingsDlg(UI::Window *parent, World &world, const ThemeManager &themeManager);
	~MapSettingsDlg();

	void OnOK();
	void OnCancel();
};
