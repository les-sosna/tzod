#include "inc/ui/DataSource.h"
#include "inc/ui/StateContext.h"
#include "inc/ui/ListBase.h"

using namespace UI;

const std::shared_ptr<StaticValue<bool>>& detail::StaticConstants<bool>::True()
{
	static auto value = std::make_shared<StaticValue<bool>>(true);
	return value;
}

const std::shared_ptr<StaticValue<bool>>& detail::StaticConstants<bool>::False()
{
	static auto value = std::make_shared<StaticValue<bool>>(false);
	return value;
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
