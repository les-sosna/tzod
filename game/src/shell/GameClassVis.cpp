#include "GameClassVis.h"
#include <gc/TypeSystem.h>
#include <render/WorldView.h>
#include <ui/LayoutContext.h>
#include <ui/StateContext.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>

GameClassVis::GameClassVis(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView)
	: UI::Window(manager)
	, _worldView(worldView)
	, _world(RectRB{-2, -2, 2, 2})
	, _texSelection(texman.FindSprite("ui/selection"))
{
}

void GameClassVis::SetGameClass(ObjectType type)
{
	_world.Clear();
	RTTypes::Inst().CreateActor(_world, type, 0, 0);
}

void GameClassVis::Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	if (auto typeName = reinterpret_cast<const std::string*>(sc.GetDataContext()))
	{
		_world.Clear();
		RTTypes::Inst().CreateActor(_world, RTTypes::Inst().GetTypeByName(*typeName), 0, 0);
	}

	RectRB viewport = { 0, 0, (int) lc.GetPixelSize().x, (int) lc.GetPixelSize().y };
	vec2d eye{ 0, 0 };
	float zoom = 1.f;
	bool editorMode = true;
	bool drawGrid = false;
	bool nightMode = false;
	_worldView.Render(dc, _world, viewport, eye, zoom, editorMode, drawGrid, nightMode);

	FRECT sel = MakeRectRB(vec2d{}, lc.GetPixelSize());
	if (sc.GetState() == "Focused")
	{
		dc.DrawSprite(sel, _texSelection, 0xffffffff, 0);
		dc.DrawBorder(sel, _texSelection, 0xffffffff, 0);
	}
	else if (sc.GetState() == "Unfocused")
	{
		dc.DrawBorder(sel, _texSelection, 0xffffffff, 0);
	}
	else if (sc.GetState() == "Hover")
	{
		dc.DrawSprite(sel, _texSelection, 0xffffffff, 0);
	}
}
