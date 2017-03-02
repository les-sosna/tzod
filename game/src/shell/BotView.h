#pragma once
#include <ui/Window.h>
#include <memory>

class ConfVarTable;
class ConfPlayerAI;

class BotView : public UI::Window
{
public:
	explicit BotView(TextureManager &texman);

	void SetBotConfig(ConfVarTable &botConf, TextureManager &texman);

	// UI::Window
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;

private:
	size_t _texSkin = -1;
	size_t _texRank;
	size_t _font;

	std::unique_ptr<ConfPlayerAI> _botConfCache;
};
