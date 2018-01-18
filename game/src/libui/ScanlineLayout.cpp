#include "inc/ui/LayoutContext.h"
#include "inc/ui/ScanlineLayout.h"
#include "inc/ui/WindowIterator.h"

using namespace UI;

FRECT ScanlineLayout::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	auto childIt = FindWindowChild(*this, child);
	assert(childIt != end(*this));

	vec2d pxElementSize = ToPx(_elementSize, lc);

	auto childIndex = std::distance(begin(*this), childIt);
	auto numColumns = std::max(1, int(lc.GetPixelSize().x / pxElementSize.x));
	int row = childIndex / numColumns;
	int column = childIndex - row * numColumns;

	float pxContentWidth = (float)numColumns * pxElementSize.x;
	float pxAlignCenterOffset = std::floor((lc.GetPixelSize().x - pxContentWidth) / 2);
	auto offset = vec2d{ pxAlignCenterOffset + (float)column * pxElementSize.x, (float)row * pxElementSize.y };
	return MakeRectWH(offset, pxElementSize);
}

vec2d ScanlineLayout::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	vec2d pxElementSize = ToPx(_elementSize, scale);
	auto numColumns = std::max(1, int(layoutConstraints.maxPixelSize.x / pxElementSize.x));
	int numRows = (GetChildrenCount() + numColumns - 1) / numColumns;
	return pxElementSize * vec2d{ (float)numColumns, (float)numRows };
}

std::shared_ptr<Window> ScanlineLayout::GetNavigateTarget(const LayoutContext &lc, const DataContext &dc, Navigate navigate)
{
	auto focusChild = GetFocus();
	auto childIt = std::find(begin(*this), end(*this), focusChild);
	assert(childIt != end(*this));
	auto childIndex = std::distance<WindowConstIterator>(begin(*this), childIt);
	auto numColumns = std::max(1, int(lc.GetPixelSize().x / ToPx(_elementSize, lc).x));

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
	case Navigate::Begin:
		return *begin(*this);
	case Navigate::End:
		return *rbegin(*this);
	default:
		break;
	}
	return nullptr;
}

bool ScanlineLayout::CanNavigate(Navigate navigate, const LayoutContext &lc, const DataContext &dc) const
{
	return !!const_cast<ScanlineLayout*>(this)->GetNavigateTarget(lc, dc, navigate);
}

void ScanlineLayout::OnNavigate(Navigate navigate, NavigationPhase phase, const LayoutContext &lc, const DataContext &dc)
{
	if (NavigationPhase::Started == phase)
	{
		if (auto newFocus = GetNavigateTarget(lc, dc, navigate))
		{
			SetFocus(std::move(newFocus));
		}
	}
}
