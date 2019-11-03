#include "UITestDesktop.h"
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/Rectangle.h>
#include <ui/Text.h>

using namespace UI::DataSourceAliases;


UITestDesktop::UITestDesktop()
	: _testRect(std::make_shared<UI::Rectangle>())
	, _testText(std::make_shared<UI::Text>())
	, _testButton(std::make_shared<UI::Button>())
{
	_testRect->SetTexture("gui_splash");
	AddFront(_testRect);

	_testText->SetText("Hello World!"_txt);
	AddFront(_testText);

	_testButton->SetText("Push me"_txt);
	_testButton->eventClick = [] {};
	AddFront(_testButton);
}

UI::WindowLayout UITestDesktop::GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	float scale = lc.GetScaleCombined();
	vec2d size = lc.GetPixelSize();

	if (_testRect.get() == &child)
	{
		return UI::WindowLayout{ UI::CanvasLayout({ 0, 42 }, { size.x, 384 }, scale), 1, true };
	}
	else if (_testText.get() == &child)
	{
		return UI::WindowLayout{ MakeRectWH({1, 1}), 1, true };
	}
	else if (_testButton.get() == &child)
	{
		return UI::WindowLayout{ MakeRectWH(Vec2dFloor((size - child.GetSize() * scale) / 2), Vec2dFloor(child.GetSize() * scale)), 1, true };
	}

	assert(false);
	return {};
}
