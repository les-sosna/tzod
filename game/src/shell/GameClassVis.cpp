#include "GameClassVis.h"
#include <gc/TypeSystem.h>
#include <ui/LayoutContext.h>
#include <render/WorldView.h>

GameClassVis::GameClassVis(UI::LayoutManager &manager, WorldView &worldView)
	: UI::Window(manager)
	, _world(RectRB{-2, -2, 2, 2})
	, _worldView(worldView)
{
}

void GameClassVis::SetGameClass(ObjectType type)
{
	_world.Clear();
	RTTypes::Inst().CreateActor(_world, type, 0, 0);
}

void GameClassVis::Draw(const UI::LayoutContext &lc, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	RectRB viewport = { 0, 0, (int) lc.GetPixelSize().x, (int) lc.GetPixelSize().y };
	vec2d eye{ 0, 0 };
	float zoom = 1.f;
	bool editorMode = false;
	bool drawGrid = false;
	bool nightMode = false;
	_worldView.Render(dc, _world, viewport, eye, zoom, editorMode, drawGrid, nightMode);
}
