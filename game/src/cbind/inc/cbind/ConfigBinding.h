#pragma once
#include <memory>
#include <string_view>

class ConfVarString;
namespace UI
{
	template<class T> struct LayoutData;
}

std::shared_ptr<UI::LayoutData<std::string_view>> ConfBind(const ConfVarString &confString);
