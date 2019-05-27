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

bool Dialog::CanNavigate(Navigate navigate, const LayoutContext &lc) const
{
	return Navigate::Back == navigate;
}

void Dialog::OnNavigate(Navigate navigate, NavigationPhase phase, const LayoutContext &lc)
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

