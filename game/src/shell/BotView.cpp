#include "BotView.h"
#include "inc/shell/Config.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

BotView::BotView(UI::LayoutManager &manager, TextureManager &texman)
	: UI::Window(manager)
	, _texRank(texman.FindSprite("rank_mark"))
{
}

void BotView::SetBotConfig(ConfVarTable &botConf, TextureManager &texman)
{
	_botConfCache.reset(new ConfPlayerAI(&botConf));
	_texSkin = texman.FindSprite(std::string("skin/") + _botConfCache->skin.Get());
}

void BotView::Draw(bool hovered, bool focused, bool enabled, vec2d size, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	UI::Window::Draw(hovered, focused, enabled, size, ic, dc, texman);

	if (_botConfCache)
	{
		FRECT rect = { 0, 0, 64, 64 };
		dc.DrawSprite(rect, _texSkin, 0xffffffff, 0);
	}
}

