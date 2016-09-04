#include "inc/ui/DataSource.h"
#include "inc/ui/StateContext.h"
#include "inc/ui/ListBase.h"

using namespace UI;

SpriteColor StaticColor::GetValue(const StateContext &sc) const
{
	return _color;
}

SpriteColor ColorMap::GetValue(const StateContext &sc) const
{
	auto found = _colorMap.find(sc.GetState());
	return _colorMap.end() != found ? found->second : _defaultColor;
}

const std::string& StaticText::GetValue(const StateContext &sc) const
{
	return _text;
}

const std::string& ListDataSourceBinding::GetValue(const StateContext &sc) const
{
	static std::string empty;
	auto listDataSource = reinterpret_cast<const ListDataSource*>(sc.GetDataContext());
	return _column < listDataSource->GetSubItemCount(sc.GetItemIndex()) ?
		listDataSource->GetItemText(sc.GetItemIndex(), _column) : empty;
}
