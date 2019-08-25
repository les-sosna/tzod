#pragma once
#include "detail/DefaultCamera.h"
#include "detail/QuickActions.h"
#include <gc/Object.h>
#include <gc/ObjPtr.h>
#include <ui/Navigation.h>
#include <ui/PointerInput.h>
#include <ui/Texture.h>
#include <ui/Window.h>

class LangCache;
class PropertyList;
class TextureManager;
class EditorConfig;
class EditorContext;
class World;
class WorldView;
class RenderScheme;
class GameClassVis;

class GC_MovingObject;

class EditorWorldView
	: public UI::WindowContainer
	, private UI::TimeStepping
	, private UI::ScrollSink
	, private UI::PointerSink
	, private UI::KeyboardSink
	, private UI::NavigationSink
{
public:
	EditorWorldView(UI::TimeStepManager &manager,
		TextureManager &texman,
		EditorContext &editorContext,
		WorldView &worldView,
		EditorConfig &conf,
		LangCache &lang,
		Plat::ConsoleBuffer &logger);
	virtual ~EditorWorldView();

	void Select(GC_Object *object, bool bSelect);
	void SelectNone();
	void SetCurrentType(ObjectType currentType) { _currentType = currentType; }
	void SetSelectOnly(bool selectOnly) { _selectOnly = selectOnly; }

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
	GC_MovingObject* PickEdObject(const RenderScheme &rs, World &world, const vec2d &pt) const;
	void EraseAt(vec2d worldPos);
	void CreateAt(vec2d worldPos, bool defaultProperties);
	void ActionOrCreateAt(vec2d worldPos, bool defaultProperties);
	void ActionOrSelectAt(vec2d worldPos);
	bool CanCreateAt(vec2d worldPos) const;
	FRECT GetNavigationOrigin() const;
	WorldCursor GetCursor() const;
	void EnsureVisible(const UI::LayoutContext &lc, FRECT worldRect);

	// UI::ScrollSink
	void OnScroll(TextureManager &texman, const UI::InputContext &ic, const UI::LayoutContext &lc, const UI::DataContext &dc, vec2d scrollOffset, bool precise) override;
	void EnsureVisible(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, FRECT pxFocusRect) override {}

	// UI::PointerSink
	bool OnPointerDown(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnPointerUp(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnPointerMove(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, bool captured) override;
	void OnTap(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// UI::KeyboardSink
	bool OnKeyPressed(const UI::InputContext &ic, Plat::Key key) override;

	// UI::NavigationSink
	bool CanNavigate(TextureManager& texman, const UI::InputContext& ic, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate) const override;
	void OnNavigate(TextureManager& texman, const UI::InputContext& ic, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate, UI::NavigationPhase phase) override;

	// UI::Window
	void OnTimeStep(const Plat::Input &input, bool focused, float dt) override;
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const override;
	UI::NavigationSink* GetNavigationSink() override { return this; }
	ScrollSink* GetScrollSink() override { return this; }
	PointerSink* GetPointerSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override { return this; }

	EditorConfig &_conf;
	vec2d _virtualPointer;
	vec2d _prevPointerPosition;
	DefaultCamera _defaultCamera;
	std::shared_ptr<PropertyList> _propList;
	UI::Texture _fontSmall = "font_small";
	UI::Texture _texSelection = "ui/selection";

	ObjPtr<GC_Object> _selectedObject;
	ObjPtr<GC_Object> _recentlyCreatedObject;
	int  _capturedButton = 0;

	ObjectType _currentType = INVALID_OBJECT_TYPE;
	bool _selectOnly = false;

	World &_world;
	WorldView &_worldView;
	QuickActions _quickActions;
};
