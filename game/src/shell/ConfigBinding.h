#pragma once
#include <memory>
#include <string>

class ConfVarString;
namespace UI
{
	template<class T> struct LayoutData;
}

std::shared_ptr<UI::LayoutData<const std::string&>> ConfBind(ConfVarString &confString);
