#pragma once
#include "QuickActions.h"
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
	, private UI::ScrollSink
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
	QuickActions _quickActions;

	void OnKillSelected(World &world, GC_Object *sender, void *param);
	void OnMoveSelected(World &world, GC_Object *sender, void *param);

public:
	EditorLayout(UI::LayoutManager &manager,
		TextureManager &texman,
		World &world,
		WorldView &worldView,
		const DefaultCamera &defaultCamera,
		ConfCache &conf,
		LangCache &lang,
		UI::ConsoleBuffer &logger);
	virtual ~EditorLayout();

	void Select(GC_Object *object, bool bSelect);
	void SelectNone();

protected:
	void OnChangeObjectType(int index);
	void OnChangeUseLayers();

	// UI::ScrollSink
	void OnScroll(UI::InputContext &ic, vec2d size, float scale, vec2d pointerPosition, vec2d offset) override;

	// UI::PointerSink
	bool OnPointerDown(UI::InputContext &ic, vec2d size, float scale, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerUp(UI::InputContext &ic, vec2d size, float scale, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerMove(UI::InputContext &ic, vec2d size, float scale, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID, bool captured) override;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;

	// UI::Window
	FRECT GetChildRect(vec2d size, float scale, const Window &child) const override;
	void Draw(const UI::LayoutContext &lc, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	ScrollSink* GetScrollSink() override { return this; }
	PointerSink* GetPointerSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override { return this; }
};
