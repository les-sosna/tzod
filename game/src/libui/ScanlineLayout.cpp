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
	auto nColumns = int(lc.GetPixelSize().x / pxElementSize.x);
	int row = childIndex / nColumns;
	int column = childIndex - row * nColumns;

	auto offset = vec2d{ (float)row * pxElementSize.y, (float)column * pxElementSize.x };
	return MakeRectWH(offset, pxElementSize);
}
