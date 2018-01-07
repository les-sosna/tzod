#include "inc/ui/LayoutContext.h"
#include "inc/ui/ScanlineLayout.h"
#include "inc/ui/WindowIterator.h"

using namespace UI;

static int hackNumColumns = 1;

FRECT ScanlineLayout::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	auto childIt = FindWindowChild(*this, child);
	assert(childIt != end(*this));

	vec2d pxElementSize = ToPx(_elementSize, lc);

	auto childIndex = std::distance(begin(*this), childIt);
	auto numColumns = std::max(1, int(lc.GetPixelSize().x / pxElementSize.x));
	int row = childIndex / numColumns;
	int column = childIndex - row * numColumns;

	hackNumColumns = numColumns;

	auto offset = vec2d{ (float)column * pxElementSize.x, (float)row * pxElementSize.y };
	return MakeRectWH(offset, pxElementSize);
}

vec2d ScanlineLayout::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	int numRows = (GetChildrenCount() + hackNumColumns - 1) / hackNumColumns;

	return ToPx(_elementSize, scale) * vec2d { (float)hackNumColumns, (float)numRows };
}

std::shared_ptr<Window> ScanlineLayout::GetNavigateTarget(const DataContext &dc, Navigate navigate)
{
	auto focusChild = GetFocus();
	auto childIt = std::find(begin(*this), end(*this), focusChild);
	assert(childIt != end(*this));
	auto childIndex = std::distance<WindowConstIterator>(begin(*this), childIt);
	int numColumns = hackNumColumns;
	int row = childIndex / numColumns;
	int column = childIndex - row * numColumns;

	switch (navigate)
	{
	case Navigate::Prev:
		return GetPrevFocusChild(*this, dc);
	case Navigate::Next:
		return GetNextFocusChild(*this, dc);
	case Navigate::Up:
		childIndex -= numColumns;
		if (childIndex >= 0)
			return *(begin(*this) + childIndex);
		break;
	case Navigate::Down:
		childIt += numColumns;
		if (childIt < end(*this))
			return *childIt;
		break;
	case Navigate::Left:
		return GetPrevFocusChild(*this, dc);
	case Navigate::Right:
		return GetNextFocusChild(*this, dc);
	}
	return nullptr;
}

bool ScanlineLayout::CanNavigate(Navigate navigate, const DataContext &dc) const
{
	return !!const_cast<ScanlineLayout*>(this)->GetNavigateTarget(dc, navigate);
}

void ScanlineLayout::OnNavigate(Navigate navigate, NavigationPhase phase, const DataContext &dc)
{
	if (NavigationPhase::Started == phase)
	{
		if (auto newFocus = GetNavigateTarget(dc, navigate))
		{
			SetFocus(std::move(newFocus));
		}
	}
}
