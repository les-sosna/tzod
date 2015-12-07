#include "pch.h"
#include "WinStoreKeys.h"
#include <ui/Keys.h>

UI::Key MapWinStoreKeyCode(Windows::System::VirtualKey platformKey, bool isExtended)
{
	if (platformKey == Windows::System::VirtualKey::Enter && isExtended)
	{
		return UI::Key::NumEnter;
	}

	switch (platformKey)
	{
#define GEN_KEY_ENTRY(platformKey, uiKey) case platformKey: return uiKey;
#include "WinStoreKeys.gen"
#undef GEN_KEY_ENTRY
	default:
		break;
	}

	return UI::Key::Unknown;
}
