#include "DataSource.h"

namespace UI
{
	class List;

	class HasSelection : public UI::LayoutData<bool>
	{
	public:
		HasSelection(std::weak_ptr<UI::List> list);

		// UI::LayoutData<bool>
		bool GetLayoutValue(const UI::DataContext &dc) const override;

	private:
		std::weak_ptr<UI::List> _list;
	};
}
