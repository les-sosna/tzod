#include "inc/ui/DataSource.h"
#include "inc/ui/Edit.h"
#include "inc/ui/EditableText.h"
#include "inc/ui/Keys.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/ScrollView.h"
#include "inc/ui/StateContext.h"

using namespace UI;

static const auto c_backgroundFrame = std::make_shared<StateBinding<unsigned int>>(0, // default
	StateBinding<unsigned int>::MapType{ { "Disabled", 1 } });

Edit::Edit()
  : _background(std::make_shared<Rectangle>())
  , _scrollView(std::make_shared<ScrollView>())
  , _editable(std::make_shared<EditableText>())
{
	_background->SetTexture("ui/edit");
	_background->SetDrawBorder(true);
	_background->SetFrame(c_backgroundFrame);
	AddFront(_background);
	AddFront(_scrollView);
	SetFocus(_scrollView);
	_scrollView->SetContent(_editable);
	_scrollView->SetFocus(_editable);
	_scrollView->SetHorizontalScrollEnabled(true);
	_scrollView->SetVerticalScrollEnabled(false);

	Resize(100, 16);
}

bool Edit::OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button)
{
	return true;
}

void Edit::OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured)
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
	return MakeRectWH(lc.GetPixelSize());
}

vec2d Edit::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	return _editable->GetContentSize(texman, dc, scale);
}
