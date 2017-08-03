#pragma once
#include <string_view>

namespace UI
{
	enum class Key;
}

std::string_view GetKeyName(UI::Key code);
UI::Key GetKeyCode(std::string_view name);
