#include "ConfigBinding.h"
#include <ui/DataSource.h>
#include <config/ConfigBase.h>

namespace UI
{
	class StateContext;
}

namespace
{
	class ConfTextSourceAdapter : public UI::TextSource
	{
	public:
		ConfTextSourceAdapter(ConfVarString &confString) : _confString(confString) {}

		const std::string& GetText(const UI::StateContext &sc) const override
		{
			return _confString.Get();
		}

	private:
		ConfVarString &_confString;
	};
}

std::shared_ptr<UI::TextSource> ConfBind(ConfVarString &confString)
{
	return std::make_shared<ConfTextSourceAdapter>(confString);
}
