#include "inc/ui/Dialog.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Rectangle.h"
#include <plat/Keys.h>

using namespace UI;

Dialog::Dialog()
	: _background(std::make_shared<Rectangle>())
{
	_background->SetTexture("ui/window");
	_background->SetDrawBorder(true);
	_background->SetDrawBackground(true);
	AddFront(_background);
}

void Dialog::Close(int result)
{
	if (OnClose(result))
	{
		if (eventClose)
			eventClose(result);
	}
}

WindowLayout Dialog::GetChildLayout(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, const Window& child) const
{
	if (_background.get() == &child)
	{
		return WindowLayout{ MakeRectWH(lc.GetPixelSize()), 1, true };
	}
	assert(false);
	return {};
}

bool Dialog::CanNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate) const
{
	return Navigate::Back == navigate;
}

void Dialog::OnNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate, NavigationPhase phase)
{
	if (Navigate::Back == navigate && NavigationPhase::Completed == phase)
	{
		Close(_resultCancel);
	}
}

bool Dialog::OnKeyPressed(const Plat::Input &input, const InputContext &ic, Plat::Key key)
{
	switch( key )
	{
	case Plat::Key::Enter:
	case Plat::Key::NumEnter:
		Close(_resultOK);
		break;
	default:
		return false;
	}
	return true;
}

