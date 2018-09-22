#pragma once

namespace Plat
{
	enum class Key;
}

Plat::Key MapGlfwKeyCode(int platformKey);
int UnmapGlfwKeyCode(Plat::Key key);
