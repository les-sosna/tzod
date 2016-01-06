#pragma once
#include <ui/Window.h>
#include <functional>

class LangCache;
class ConfCache;
class DefaultCamera;
class GC_Object;
class PropertyList;
class ServiceEditor;
class World;
class WorldView;
struct lua_State;

namespace UI
{
	template <class, class> class ListAdapter;
	class ListDataSourceDefault;
	class ComboBox;
	class ConsoleBuffer;
	class Text;
}

class EditorLayout : public UI::Window
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	ConfCache &_conf;
	LangCache &_lang;
	UI::ConsoleBuffer &_logger;
	const DefaultCamera &_defaultCamera;
	PropertyList *_propList;
	ServiceEditor    *_serviceList;
	UI::Text         *_layerDisp;
	DefaultComboBox  *_typeList;
	UI::Text         *_help;
	size_t        _fontSmall;

	size_t       _texSelection;

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
	EditorLayout(UI::Window *parent, World &world, WorldView &worldView, const DefaultCamera &defaultCamera, lua_State *globL, ConfCache &conf, LangCache &lang, UI::ConsoleBuffer &logger);
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
	bool OnKeyPressed(UI::Key key);
	void OnSize(float width, float height);
	void OnVisibleChange(bool visible, bool inherited);

	void OnChangeObjectType(int index);
	void OnChangeUseLayers();
};
