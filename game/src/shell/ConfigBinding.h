#pragma once
#include <memory>
#include <string>

class ConfVarString;
namespace UI
{
	template<class T> struct DataSource;
}

std::shared_ptr<UI::DataSource<const std::string&>> ConfBind(ConfVarString &confString);
