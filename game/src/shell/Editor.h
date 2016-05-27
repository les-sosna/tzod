#pragma once
#include <ui/Window.h>
#include <functional>

class LangCache;
class ConfCache;
class DefaultCamera;
class GC_Object;
class PropertyList;
class ServiceEditor;
class TextureManager;
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

class EditorLayout
	: public UI::Window
	, private UI::PointerSink
	, private UI::KeyboardSink
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	ConfCache &_conf;
	LangCache &_lang;
	UI::ConsoleBuffer &_logger;
	const DefaultCamera &_defaultCamera;
	std::shared_ptr<PropertyList> _propList;
	std::shared_ptr<ServiceEditor> _serviceList;
	std::shared_ptr<UI::Text> _layerDisp;
	std::shared_ptr<DefaultComboBox> _typeList;
	std::shared_ptr<UI::Text> _help;
	size_t _fontSmall;

	size_t _texSelection;

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
	EditorLayout(UI::LayoutManager &manager,
		TextureManager &texman,
		World &world,
		WorldView &worldView,
		const DefaultCamera &defaultCamera,
		lua_State *globL,
		ConfCache &conf,
		LangCache &lang,
		UI::ConsoleBuffer &logger);
	virtual ~EditorLayout();

	void Select(GC_Object *object, bool bSelect);
	void SelectNone();

protected:
	void OnChangeObjectType(int index);
	void OnChangeUseLayers();

	// UI::PointerSink
	void OnMouseWheel(vec2d pointerPosition, float z) override;
	bool OnPointerDown(UI::InputContext &ic, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerUp(UI::InputContext &ic, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerMove(UI::InputContext &ic, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID) override;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;

	// UI::Window
	void OnSize(float width, float height) override;
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	PointerSink* GetPointerSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override { return this; }
};
