#pragma once
#include <ui/List.h>

namespace FS
{
	class FileSystem;
}

class MapListDataSource : public UI::ListDataSourceDefault
{
public:
	MapListDataSource(FS::FileSystem &fs);
};

class MapListItem : public UI::Window
{
public:
	MapListItem();
};
