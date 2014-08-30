#pragma once
#include "List.h"

namespace FS
{
	class FileSystem;
}

namespace UI
{

class ListDataSourceMaps : public ListDataSourceDefault
{
public:
	ListDataSourceMaps(FS::FileSystem &fs);
};


} // end of namespace UI
