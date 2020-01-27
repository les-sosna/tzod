#include "GameClassVis.h"
#include <gc/TypeSystem.h>
#include <render/WorldView.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/StateContext.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

GameClassVis::GameClassVis(WorldView &worldView)
	: _worldView(worldView)
	, _world(RectRB{-2, -2, 2, 2}, false /* initField */)
{
}

void GameClassVis::SetGameClass(std::shared_ptr<UI::RenderData<std::string_view>> className)
{
	_className = std::move(className);
}

void GameClassVis::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const
{
	if (!_className)
		return;

	_world.Clear();
	auto type = RTTypes::Inst().GetTypeByName(_className->GetRenderValue(dc, sc));
	auto offset = RTTypes::Inst().GetTypeInfo(type).offset;
	RTTypes::Inst().CreateObject(_world, type, vec2d{ offset, offset });

	RectRB viewport = { 0, 0, (int) lc.GetPixelSize().x, (int) lc.GetPixelSize().y };
	vec2d eye{ offset, offset };
	float zoom = lc.GetScaleCombined();

	rc.PushClippingRect(viewport);
	rc.PushWorldTransform(ComputeWorldTransformOffset(RectToFRect(viewport), eye, zoom), zoom, 1);
	WorldViewRenderOptions options;
	options.editorMode = true;
	options.noBackground = true;
	_worldView.Render(rc, _world, options);
	rc.PopTransform();
	rc.PopClippingRect();

	FRECT sel = MakeRectRB(vec2d{}, lc.GetPixelSize());
	if (sc.GetState() == "Focused")
	{
		rc.DrawSprite(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
		rc.DrawBorder(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
	}
	else if (sc.GetState() == "Unfocused")
	{
		rc.DrawBorder(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
	}
	else if (sc.GetState() == "Hover")
	{
		rc.DrawSprite(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
	}
}
