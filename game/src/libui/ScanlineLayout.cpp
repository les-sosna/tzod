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
	int numRows = GetChildrenCount() / hackNumColumns;

	return ToPx(_elementSize, scale) * vec2d { (float)hackNumColumns, (float)numRows };
}
