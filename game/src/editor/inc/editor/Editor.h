#pragma once
#include "detail/DefaultCamera.h"
#include "detail/QuickActions.h"
#include <gc/Object.h>
#include <gc/ObjPtr.h>
#include <ui/Texture.h>
#include <ui/Window.h>
#include <functional>

class LangCache;
class PropertyList;
class TextureManager;
class EditorConfig;
class EditorContext;
class World;
class WorldView;
class RenderScheme;
class GameClassVis;

class GC_Actor;

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
	, private UI::TimeStepping
	, private UI::ScrollSink
	, private UI::PointerSink
	, private UI::KeyboardSink
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ListBox> DefaultListBox;

	EditorConfig &_conf;
	LangCache &_lang;
	DefaultCamera _defaultCamera;
	std::shared_ptr<PropertyList> _propList;
	std::shared_ptr<UI::Text> _layerDisp;
	std::shared_ptr<UI::Text> _help;
	std::shared_ptr<DefaultListBox> _typeSelector;
	UI::Texture _fontSmall = "font_small";
	UI::Texture _texSelection = "ui/selection";

	ObjPtr<GC_Object> _selectedObject;
	bool _isObjectNew = false;
	int  _capturedButton = 0;
	World &_world;
	WorldView &_worldView;
	QuickActions _quickActions;

public:
	EditorLayout(UI::LayoutManager &manager,
		EditorContext &editorContext,
		WorldView &worldView,
		EditorConfig &conf,
		LangCache &lang,
		UI::ConsoleBuffer &logger);
	virtual ~EditorLayout();

	void Select(GC_Object *object, bool bSelect);
	void SelectNone();

private:
	vec2d CanvasToWorld(const UI::LayoutContext &lc, vec2d canvasPos) const;
	vec2d WorldToCanvas(const UI::LayoutContext &lc, vec2d worldPos) const;
	FRECT WorldToCanvas(const UI::LayoutContext &lc, FRECT worldRect) const;
	GC_Actor* PickEdObject(const RenderScheme &rs, World &world, const vec2d &pt) const;
	ObjectType GetCurrentType() const;
	void EraseAt(vec2d worldPos);
	void CreateAt(vec2d worldPos, bool defaultProperties);
	void ActionOrSelectOrCreateAt(vec2d worldPos, bool defaultProperties);
	vec2d AlignToGrid(vec2d worldPos) const;

	void OnChangeUseLayers();

	// UI::ScrollSink
	void OnScroll(TextureManager &texman, const UI::InputContext &ic, const UI::LayoutContext &lc, const UI::DataContext &dc, vec2d pointerPosition, vec2d offset) override;

	// UI::PointerSink
	bool OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID, bool captured) override;
	void OnTap(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;

	// UI::Window
	void OnTimeStep(const UI::InputContext &ic, float dt) override;
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;
	ScrollSink* GetScrollSink() override { return this; }
	PointerSink* GetPointerSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override { return this; }
};
