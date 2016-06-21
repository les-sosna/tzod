#pragma once
#include <ui/Window.h>
#include <memory>

class ConfVarTable;
class ConfPlayerAI;

class BotView : public UI::Window
{
public:
	BotView(UI::LayoutManager &manager, TextureManager &texman);

	void SetBotConfig(ConfVarTable &botConf, TextureManager &texman);

	// UI::Window
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	size_t _texSkin = -1;
	size_t _texRank;

	std::unique_ptr<ConfPlayerAI> _botConfCache;
};
