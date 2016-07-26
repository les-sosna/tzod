#include "inc/ui/DataSource.h"
#include "inc/ui/StateContext.h"

using namespace UI;

SpriteColor StaticColor::GetColor(const StateContext &sc) const
{
	return _color;
}

SpriteColor ColorMap::GetColor(const StateContext &sc) const
{
	auto found = _colorMap.find(sc.GetState());
	return _colorMap.end() != found ? found->second : _defaultColor;
}
