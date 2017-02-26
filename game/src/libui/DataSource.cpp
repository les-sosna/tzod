#include "inc/ui/DataSource.h"
#include "inc/ui/DataContext.h"
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

const std::string& StaticText::GetValue(const DataContext &dc) const
{
	return _text;
}

const std::string& StaticText::GetValue(const DataContext &dc, const StateContext &sc) const
{
	return _text;
}

const std::string& ListDataSourceBinding::GetValue(const DataContext &dc) const
{
	static std::string empty;
	auto listDataSource = reinterpret_cast<const ListDataSource*>(dc.GetDataContext());
	return _column < listDataSource->GetSubItemCount(dc.GetItemIndex()) ?
		listDataSource->GetItemText(dc.GetItemIndex(), _column) : empty;
}

const std::string& ListDataSourceBinding::GetValue(const DataContext &dc, const StateContext &sc) const
{
	return ListDataSourceBinding::GetValue(dc);
}
