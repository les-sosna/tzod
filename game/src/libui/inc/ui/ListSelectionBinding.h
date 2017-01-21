#include "DataSource.h"

namespace UI
{
	class List;

	class HasSelection : public UI::DataSource<bool>
	{
	public:
		HasSelection(std::weak_ptr<UI::List> list);

		// UI::DataSource<bool>
		bool GetValue(const UI::StateContext &sc) const override;

	private:
		std::weak_ptr<UI::List> _list;
	};
}
