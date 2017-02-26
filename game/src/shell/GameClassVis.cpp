#include "GameClassVis.h"
#include <gc/TypeSystem.h>
#include <render/WorldView.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/StateContext.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

GameClassVis::GameClassVis(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView)
	: UI::Window(manager)
	, _worldView(worldView)
	, _world(RectRB{-2, -2, 2, 2})
	, _texSelection(texman.FindSprite("ui/selection"))
{
}

void GameClassVis::SetGameClass(std::shared_ptr<UI::RenderData<const std::string&>> className)
{
	_className = std::move(className);
}

void GameClassVis::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman) const
{
	if (!_className)
		return;

	_world.Clear();
	RTTypes::Inst().CreateActor(_world, RTTypes::Inst().GetTypeByName(_className->GetValue(dc, sc)), vec2d{});

	RectRB viewport = { 0, 0, (int) lc.GetPixelSize().x, (int) lc.GetPixelSize().y };
	vec2d eye{ 0, 0 };
	float zoom = lc.GetScale();
	bool editorMode = true;
	bool drawGrid = false;
	bool nightMode = false;
	_worldView.Render(rc, _world, viewport, eye, zoom, editorMode, drawGrid, nightMode);

	FRECT sel = MakeRectRB(vec2d{}, lc.GetPixelSize());
	if (sc.GetState() == "Focused")
	{
		rc.DrawSprite(sel, _texSelection, 0xffffffff, 0);
		rc.DrawBorder(sel, _texSelection, 0xffffffff, 0);
	}
	else if (sc.GetState() == "Unfocused")
	{
		rc.DrawBorder(sel, _texSelection, 0xffffffff, 0);
	}
	else if (sc.GetState() == "Hover")
	{
		rc.DrawSprite(sel, _texSelection, 0xffffffff, 0);
	}
}
