#pragma once
#include <ui/List.h>

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ConsoleBuffer;
}

class ListDataSourceMaps : public UI::ListDataSourceDefault
{
public:
	ListDataSourceMaps(FS::FileSystem &fs, UI::ConsoleBuffer &logger);
};

