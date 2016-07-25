#include "UITestDesktop.h"
#include <ui/Rectangle.h>
#include <ui/Text.h>

UITestDesktop::UITestDesktop(UI::LayoutManager &manager, TextureManager &texman)
	: UI::Window(manager)
	, _testRect(std::make_shared<UI::Rectangle>(manager))
	, _testText(std::make_shared<UI::Text>(manager, texman))
{
	_testRect->SetTexture(texman, "gui_splash", false);
	AddFront(_testRect);

	_testText->SetText(texman, "Hello World!");
	AddFront(_testText);
}

FRECT UITestDesktop::GetChildRect(vec2d size, float scale, const Window &child) const
{
	if (_testRect.get() == &child)
	{
		return UI::CanvasLayout({ 0, 42 }, { size.x, 384 }, scale);
	}

	return UI::Window::GetChildRect(size, scale, child);
}
