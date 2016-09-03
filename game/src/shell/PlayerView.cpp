#include "PlayerView.h"
#include "inc/shell/Config.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

PlayerView::PlayerView(UI::LayoutManager &manager, TextureManager &texman)
	: UI::Window(manager)
{
}

void PlayerView::SetPlayerConfig(ConfVarTable &playerConf, TextureManager &texman)
{
	_playerConfCache.reset(new ConfPlayerLocal(&playerConf));
	_texSkin = texman.FindSprite(std::string("skin/") + _playerConfCache->skin.Get());
}

void PlayerView::Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	if (_playerConfCache)
	{
		FRECT rect = { 0, 0, 64, 64 };
		dc.DrawSprite(rect, _texSkin, 0xffffffff, 0);
	}
}

