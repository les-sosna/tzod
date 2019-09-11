#pragma once
#include <string_view>

namespace Plat
{
	enum class Key;
}

std::string_view GetKeyName(Plat::Key code);
Plat::Key GetKeyCode(std::string_view name);
