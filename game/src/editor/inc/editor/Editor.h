#pragma once
#include "detail/DefaultCamera.h"
#include "detail/QuickActions.h"
#include <gc/Object.h>
#include <gc/ObjPtr.h>
#include <ui/Navigation.h>
#include <ui/PointerInput.h>
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

namespace Plat
{
	class ConsoleBuffer;
}

namespace UI
{
	class ListDataSourceDefault;
	class CheckBox;
	class Text;
	class ListBox;
	class StackLayout;
	template<class, class> class ListAdapter;
}

struct EditorCommands
{
	std::function<void()> playMap;
};

class EditorLayout
	: public UI::Window
	, private UI::TimeStepping
	, private UI::ScrollSink
	, private UI::PointerSink
	, private UI::KeyboardSink
	, private UI::NavigationSink
{
public:
	EditorLayout(UI::TimeStepManager &manager,
		TextureManager &texman,
		EditorContext &editorContext,
		WorldView &worldView,
		EditorConfig &conf,
		LangCache &lang,
		EditorCommands commands,
		Plat::ConsoleBuffer &logger);
	virtual ~EditorLayout();

	void Select(GC_Object *object, bool bSelect);
	void SelectNone();

private:
	struct WorldCursor
	{
		enum class Type
		{
			None,
			Create,
			Obstructed,
			Action
		};

		FRECT bounds;
		Type cursorType;
	};
	vec2d CanvasToWorld(const UI::LayoutContext &lc, vec2d canvasPos) const;
	FRECT CanvasToWorld(vec2d worldTransformOffset, float worldTransformScale, FRECT canvasRect) const;
	vec2d WorldToCanvas(vec2d worldTransformOffset, float worldTransformScale, vec2d worldPos) const;
	FRECT WorldToCanvas(vec2d worldTransformOffset, float worldTransformScale, FRECT worldRect) const;
	GC_Actor* PickEdObject(const RenderScheme &rs, World &world, const vec2d &pt) const;
	ObjectType GetCurrentType() const;
	void EraseAt(vec2d worldPos);
	void CreateAt(vec2d worldPos, bool defaultProperties);
	void ActionOrCreateAt(vec2d worldPos, bool defaultProperties);
	void ActionOrSelectAt(vec2d worldPos);
	bool CanCreateAt(vec2d worldPos) const;
	FRECT GetNavigationOrigin() const;
	WorldCursor GetCursor() const;
	void ChooseNextType();
	void ChoosePrevType();
	void EnsureVisible(const UI::LayoutContext &lc, FRECT worldRect);

	void OnChangeUseLayers();

	// UI::ScrollSink
	void OnScroll(TextureManager &texman, const UI::InputContext &ic, const UI::LayoutContext &lc, const UI::DataContext &dc, vec2d scrollOffset, bool precise) override;
	void EnsureVisible(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, FRECT pxFocusRect) override {}

	// UI::PointerSink
	bool OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, bool captured) override;
	void OnTap(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, Plat::Key key) override;

	// UI::NavigationSink
	bool CanNavigate(UI::Navigate navigate, const UI::LayoutContext &lc, const UI::DataContext &dc) const override;
	void OnNavigate(UI::Navigate navigate, UI::NavigationPhase phase, const UI::LayoutContext &lc, const UI::DataContext &dc) override;

	// UI::Window
	void OnTimeStep(const UI::InputContext &ic, float dt) override;
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;
	bool HasNavigationSink() const override { return true; }
	UI::NavigationSink* GetNavigationSink() override { return this; }
	bool HasScrollSink() const override { return true; }
	ScrollSink* GetScrollSink() override { return this; }
	bool HasPointerSink() const override { return true; }
	PointerSink* GetPointerSink() override { return this; }
	bool HasKeyboardSink() const override { return true; }
	KeyboardSink *GetKeyboardSink() override { return this; }

	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ListBox> DefaultListBox;

	EditorConfig &_conf;
	LangCache &_lang;
	EditorCommands _commands;
	vec2d _virtualPointer;
	DefaultCamera _defaultCamera;
	std::shared_ptr<PropertyList> _propList;
	std::shared_ptr<UI::Text> _layerDisp;
	std::shared_ptr<UI::Text> _help;
	std::shared_ptr<UI::CheckBox> _modeSelect;
	std::shared_ptr<UI::CheckBox> _modeErase;
	std::shared_ptr<DefaultListBox> _typeSelector;
	std::shared_ptr<UI::StackLayout> _toolbar;
	UI::Texture _fontSmall = "font_small";
	UI::Texture _texSelection = "ui/selection";

	ObjPtr<GC_Object> _selectedObject;
	ObjPtr<GC_Object> _recentlyCreatedObject;
	int  _capturedButton = 0;

	World &_world;
	WorldView &_worldView;
	QuickActions _quickActions;
};
