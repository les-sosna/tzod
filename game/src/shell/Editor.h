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
	void Draw(DrawingContext &dc) const override;

	bool OnMouseWheel(float x, float y, float z) override;
	bool OnPointerDown(float x, float y, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	bool OnPointerUp(float x, float y, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	bool OnPointerMove(float x, float y, UI::PointerType pointerType, unsigned int pointerID) override;
	bool OnFocus(bool focus) override;
	bool OnKeyPressed(UI::Key key) override;
	void OnSize(float width, float height) override;
	void OnVisibleChange(bool visible, bool inherited) override;

	void OnChangeObjectType(int index);
	void OnChangeUseLayers();
};
