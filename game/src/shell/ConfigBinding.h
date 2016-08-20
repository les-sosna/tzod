#pragma once
#include <memory>

class ConfVarString;
namespace UI
{
	struct TextSource;
}

std::shared_ptr<UI::TextSource> ConfBind(ConfVarString &confString);
