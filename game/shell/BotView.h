#pragma once
#include <ui/Texture.h>
#include <ui/Window.h>
#include <memory>

class ConfVarTable;
class ConfPlayerAI;

class BotView : public UI::Window
{
public:
	explicit BotView(TextureManager &texman);

	void SetBotConfig(ConfVarTable &botConf);

	// UI::Window
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const override;

private:
	UI::Texture _texSkin;
	UI::Texture _texRank = "rank_mark";
	UI::Texture _font = "font_small";

	std::unique_ptr<ConfPlayerAI> _botConfCache;
};
