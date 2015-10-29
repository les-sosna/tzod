#pragma once

namespace UI
{
	enum class Key;
}

UI::Key MapGlfwKeyCode(int platformKey);
int UnmapGlfwKeyCode(UI::Key key);
