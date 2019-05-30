#include "inc/ui/DataSource.h"
#include "inc/ui/Edit.h"
#include "inc/ui/EditableText.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/StateContext.h"
#include <plat/Keys.h>

using namespace UI;

static const auto c_backgroundFrame = std::make_shared<StateBinding<unsigned int>>(0, // default
	StateBinding<unsigned int>::MapType{ { "Disabled", 1 } });

Edit::Edit()
	: _editable(std::make_shared<EditableText>())
{
	std::get<Rectangle>(_children).SetTexture("ui/edit");
	std::get<Rectangle>(_children).SetDrawBorder(true);
	std::get<Rectangle>(_children).SetFrame(c_backgroundFrame);

	std::get<ScrollView>(_children).SetContent(_editable);
	std::get<ScrollView>(_children).SetHorizontalScrollEnabled(true);
	std::get<ScrollView>(_children).SetVerticalScrollEnabled(false);
	std::get<ScrollView>(_children).SetStretchContent(true);

	Resize(100, 16);
}

bool Edit::OnPointerDown(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button)
{
	return true;
}

void Edit::OnPointerMove(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured)
{
}

void Edit::OnTap(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
}

void Edit::PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic, bool hovered) const
{
	sc.SetState(lc.GetEnabledCombined() ? "" : "Disabled");
}

unsigned int Edit::GetChildrenCount() const
{
	return std::tuple_size<EditBoxChildren>::value;
}

std::shared_ptr<const Window> Edit::GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const
{
	return { owner, &Edit::GetChild(index) };
}

const Window& Edit::GetChild(unsigned int index) const
{
	switch (index)
	{
	default:
		assert(false);
	case 0: return std::get<0>(_children);
	case 1: return std::get<1>(_children);
	}
}

WindowLayout Edit::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	return WindowLayout{ MakeRectWH(lc.GetPixelSize()), 1, true };
}

vec2d Edit::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	return _editable->GetContentSize(texman, dc, scale, layoutConstraints);
}

std::shared_ptr<const Window> Edit::GetFocus(const std::shared_ptr<const Window>& owner) const
{
	return { owner, &std::get<ScrollView>(_children) };
}

const Window* Edit::GetFocus() const
{
	return &std::get<ScrollView>(_children);
}
