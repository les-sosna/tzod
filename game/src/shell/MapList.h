#pragma once
#include <ui/List.h>

namespace FS
{
	class FileSystem;
}

namespace Plat
{
	class ConsoleBuffer;
}

class ListDataSourceMaps : public UI::ListDataSourceDefault
{
public:
	ListDataSourceMaps(FS::FileSystem &fs, Plat::ConsoleBuffer &logger);
};

