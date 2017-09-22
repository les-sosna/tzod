#include "MapList.h"
#include <as/AppConstants.h>
#include <fs/FileSystem.h>
#include <ui/ConsoleBuffer.h>

#include <sstream>
#include <iomanip>

MapListDataSource::MapListDataSource(FS::FileSystem &fs)
{
	auto files = fs.GetFileSystem(DIR_MAPS)->EnumAllFiles("*.map");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		std::string tmp = DIR_MAPS;
		tmp += "/";
		tmp += *it;

		std::string tmp2 = *it;
		tmp2.erase(it->length() - 4); // cut out the file extension
		int index = AddItem(tmp2);
	}

	Sort();
}

MapListItem::MapListItem()
{
}


