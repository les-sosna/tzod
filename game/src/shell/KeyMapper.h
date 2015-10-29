#pragma once
#include <string>

namespace UI
{
	enum class Key;
}

const std::string& GetKeyName(UI::Key code);
UI::Key GetKeyCode(const std::string &name);
