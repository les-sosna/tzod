#include "inc/ui/DataSource.h"
#include "inc/ui/Edit.h"
#include "inc/ui/EditableText.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/StateContext.h"

#include <algorithm>
#include <cstring>
#include <sstream>

using namespace UI;

static const auto c_backgroundFrame = std::make_shared<StateBinding<unsigned int>>(0, // default
	StateBinding<unsigned int>::MapType{ { "Disabled", 1 } });

Edit::Edit(LayoutManager &manager, TextureManager &texman)
  : _background(std::make_shared<Rectangle>())
  , _editable(std::make_shared<EditableText>(manager, texman))
{
	_background->SetTexture("ui/edit");
	_background->SetDrawBorder(true);
	_background->SetFrame(c_backgroundFrame);
	AddFront(_background);
	AddFront(_editable);
	SetFocus(_editable);

	Resize(100, 16);
}

bool Edit::OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	return true;
}

void Edit::OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured)
{
}

void Edit::OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
}

void Edit::PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic) const
{
	sc.SetState(lc.GetEnabledCombined() ? "" : "Disabled");
}

FRECT Edit::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	if (_background.get() == &child)
	{
		return MakeRectWH(lc.GetPixelSize());
	}
	return Window::GetChildRect(texman, lc, dc, child);
}

vec2d Edit::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	return _editable->GetContentSize(texman, dc, scale);
}
