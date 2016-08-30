#pragma once
#include "ui/Window.h"

namespace UI
{
	class Button;
	class Rectangle;
	class Text;
}

class UITestDesktop : public UI::Window
{
public:
	UITestDesktop(UI::LayoutManager &manager, TextureManager &texman);

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const Window &child) const override;

private:
	std::shared_ptr<UI::Rectangle> _testRect;
	std::shared_ptr<UI::Text> _testText;
	std::shared_ptr<UI::Button> _testButton;
};
