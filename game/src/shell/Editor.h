#pragma once
#include "QuickActions.h"
#include <ui/Window.h>
#include <functional>

class LangCache;
class ConfCache;
class DefaultCamera;
class GC_Object;
class PropertyList;
class TextureManager;
class World;
class WorldView;

class GameClassVis;

namespace UI
{
	class ListDataSourceDefault;
	class ConsoleBuffer;
	class Text;
	class ListBox;
	template<class, class> class ListAdapter;
}

class EditorLayout
	: public UI::Window
	, private UI::ScrollSink
	, private UI::PointerSink
	, private UI::KeyboardSink
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ListBox> DefaultListBox;

	ConfCache &_conf;
	LangCache &_lang;
	UI::ConsoleBuffer &_logger;
	const DefaultCamera &_defaultCamera;
	std::shared_ptr<PropertyList> _propList;
	std::shared_ptr<UI::Text> _layerDisp;
	std::shared_ptr<UI::Text> _help;
	std::shared_ptr<DefaultListBox> _typeSelector;
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

private:
	vec2d CanvasToWorld(vec2d canvasSize, vec2d canvasPos) const;
	vec2d WorldToCanvas(vec2d canvasSize, vec2d worldPos) const;
	FRECT WorldToCanvas(vec2d canvasSize, FRECT worldRect) const;

	void OnChangeObjectType(int index);
	void OnChangeUseLayers();

	// UI::ScrollSink
	void OnScroll(UI::InputContext &ic, UI::LayoutContext &lc, vec2d pointerPosition, vec2d offset) override;

	// UI::PointerSink
	bool OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID, bool captured) override;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;

	// UI::Window
	FRECT GetChildRect(vec2d size, float scale, const Window &child) const override;
	void Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	ScrollSink* GetScrollSink() override { return this; }
	PointerSink* GetPointerSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override { return this; }
};
