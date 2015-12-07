#pragma once

namespace UI
{
	enum class Key;
}

namespace Windows
{
	namespace System
	{
		enum class VirtualKey;
	}
}

UI::Key MapWinStoreKeyCode(Windows::System::VirtualKey platformKey, bool isExtended);
//Windows::System::VirtualKey platformKey UnmapWinStoreKeyCode(UI::Key key);
