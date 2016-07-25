#pragma once
#include "ui/Window.h"

namespace UI
{
	class Rectangle;
	class Text;
}

class UITestDesktop : public UI::Window
{
public:
	UITestDesktop(UI::LayoutManager &manager, TextureManager &texman);

	// UI::Window
	FRECT GetChildRect(vec2d size, float scale, const Window &child) const override;

private:
	std::shared_ptr<UI::Rectangle> _testRect;
	std::shared_ptr<UI::Text> _testText;
};
