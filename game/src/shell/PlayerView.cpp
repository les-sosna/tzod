#include "PlayerView.h"
#include "inc/shell/Config.h"
#include <ui/LayoutContext.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

void PlayerView::SetPlayerConfig(ConfVarTable &playerConf)
{
	_playerConfCache.reset(new ConfPlayerLocal(&playerConf));
	_texSkin = std::string("skin/").append(_playerConfCache->skin.Get());
}

void PlayerView::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const
{
	if (_playerConfCache)
	{
		vec2d pxSize = UI::ToPx(vec2d{ 64, 64 }, lc);
		rc.DrawSprite(MakeRectWH(pxSize), _texSkin.GetTextureId(texman), 0xffffffff, 0);
	}
}

