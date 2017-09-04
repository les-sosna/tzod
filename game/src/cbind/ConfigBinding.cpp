#include "inc/cbind/ConfigBinding.h"
#include <ui/DataSource.h>
#include <config/ConfigBase.h>

namespace UI
{
	class StateContext;
}

namespace
{
	class ConfTextSourceAdapter : public UI::LayoutData<std::string_view>
	{
	public:
		ConfTextSourceAdapter(const ConfVarString &confString) : _confString(confString) {}

		// UI::LayoutData<std::string_view>
		std::string_view GetLayoutValue(const UI::DataContext &dc) const override
		{
			return _confString.Get();
		}

	private:
		const ConfVarString &_confString;
	};
}

std::shared_ptr<UI::LayoutData<std::string_view>> ConfBind(const ConfVarString &confString)
{
	return std::make_shared<ConfTextSourceAdapter>(confString);
}
