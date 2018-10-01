#pragma once
#include <ui/Texture.h>
#include <ui/Window.h>
#include <memory>

class ConfVarTable;
class ConfPlayerLocal;

class PlayerView final
	: public UI::Window
{
public:
	void SetPlayerConfig(ConfVarTable &playerConf);

	// UI::Window
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;

private:
	UI::Texture _texSkin;
	std::unique_ptr<ConfPlayerLocal> _playerConfCache;
};
