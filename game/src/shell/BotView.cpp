#include "BotView.h"
#include "inc/shell/Config.h"
#include <ui/LayoutContext.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

void BotView::SetBotConfig(ConfVarTable &botConf)
{
	_botConfCache.reset(new ConfPlayerAI(&botConf));
	_texSkin = std::string("skin/") + _botConfCache->skin.Get();
}

void BotView::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	if (_botConfCache)
	{
		float pxTextHeight = ToPx(texman.GetSpriteInfo(_font.GetTextureId(texman)).pxFrameHeight, lc);
		vec2d pxSize = UI::ToPx(vec2d{ 64, 64 }, lc);
		rc.DrawSprite(MakeRectWH(vec2d{0, -pxTextHeight}, pxSize), _texSkin.GetTextureId(texman), 0xffffffff, 0);
		rc.DrawBitmapText(vec2d{ std::floor(pxSize.x / 2), pxSize.y - pxTextHeight }, lc.GetScale(), _font.GetTextureId(texman), 0x7f7f7f7f, _botConfCache->nick.Get(), alignTextCB);
	}
}

