#include "UITestDesktop.h"
#include <ui/Button.h>
#include <ui/Rectangle.h>
#include <ui/Text.h>

UITestDesktop::UITestDesktop(UI::LayoutManager &manager, TextureManager &texman)
	: UI::Window(manager)
	, _testRect(std::make_shared<UI::Rectangle>(manager))
	, _testText(std::make_shared<UI::Text>(manager, texman))
	, _testButton(std::make_shared<UI::Button>(manager, texman))
{
	_testRect->SetTexture(texman, "gui_splash", false);
	AddFront(_testRect);

	_testText->SetText(texman, "Hello World!");
	AddFront(_testText);

	_testButton->SetText(texman, "Push me");
	AddFront(_testButton);
}

FRECT UITestDesktop::GetChildRect(vec2d size, float scale, const Window &child) const
{
	if (_testRect.get() == &child)
	{
		return UI::CanvasLayout({ 0, 42 }, { size.x, 384 }, scale);
	}
	else if (_testButton.get() == &child)
	{
		return MakeRectWH(Vec2dFloor((size - child.GetSize() * scale) / 2), Vec2dFloor(child.GetSize() * scale));
	}

	return UI::Window::GetChildRect(size, scale, child);
}
