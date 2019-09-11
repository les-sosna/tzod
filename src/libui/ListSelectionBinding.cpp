#include "inc/ui/ListSelectionBinding.h"
#include "inc/ui/List.h"

using namespace UI;

HasSelection::HasSelection(std::weak_ptr<UI::List> list)
	: _list(std::move(list))
{}

bool HasSelection::GetLayoutValue(const UI::DataContext &sc) const
{
	if (auto list = _list.lock())
	{
		return list->GetCurSel() != -1;
	}
	return false;
}
