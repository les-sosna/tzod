#pragma once

namespace Plat
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

Plat::Key MapWinStoreKeyCode(Windows::System::VirtualKey platformKey, bool isExtended);
Windows::System::VirtualKey UnmapWinStoreKeyCode(Plat::Key key);
