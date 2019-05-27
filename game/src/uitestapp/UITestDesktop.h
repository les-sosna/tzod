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
	UITestDesktop();

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;

private:
	std::shared_ptr<UI::Rectangle> _testRect;
	std::shared_ptr<UI::Text> _testText;
	std::shared_ptr<UI::Button> _testButton;
};
