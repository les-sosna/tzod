#pragma once
#include <ui/Window.h>
#include <memory>

class ConfVarTable;
class ConfPlayerLocal;

class PlayerView : public UI::Window
{
public:
	void SetPlayerConfig(ConfVarTable &playerConf, TextureManager &texman);

	// UI::Window
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman) const override;

private:
	size_t _texSkin = -1;

	std::unique_ptr<ConfPlayerLocal> _playerConfCache;
};
