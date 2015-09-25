#pragma once
#include <ui/List.h>

namespace FS
{
	class FileSystem;
}

class ListDataSourceMaps : public UI::ListDataSourceDefault
{
public:
	ListDataSourceMaps(FS::FileSystem &fs);
};

