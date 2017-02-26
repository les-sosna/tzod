#include "ConfigBinding.h"
#include <ui/DataSource.h>
#include <config/ConfigBase.h>

namespace UI
{
	class StateContext;
}

namespace
{
	class ConfTextSourceAdapter : public UI::LayoutData<const std::string&>
	{
	public:
		ConfTextSourceAdapter(ConfVarString &confString) : _confString(confString) {}

		// UI::LayoutData<const std::string&>
		const std::string& GetValue(const UI::DataContext &dc) const override
		{
			return _confString.Get();
		}

	private:
		ConfVarString &_confString;
	};
}

std::shared_ptr<UI::LayoutData<const std::string&>> ConfBind(ConfVarString &confString)
{
	return std::make_shared<ConfTextSourceAdapter>(confString);
}
