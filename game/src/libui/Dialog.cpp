#include "inc/ui/Dialog.h"
#include "inc/ui/InputContext.h"
#include <plat/Keys.h>

using namespace UI;

Dialog::Dialog()
{
	SetTexture("ui/window");
	SetDrawBorder(true);
	SetDrawBackground(true);
}

void Dialog::Close(int result)
{
	if (OnClose(result))
	{
		if (eventClose)
			eventClose(result);
	}
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

bool Dialog::OnKeyPressed(const InputContext &ic, Plat::Key key)
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

