#include "inc/as/MapCollection.h"
#include "inc/as/AppConstants.h"
#include <fs/FileSystem.h>
#include <algorithm>

MapCollection::MapCollection(FS::FileSystem &fs)
	: _mapNames(fs.GetFileSystem(DIR_MAPS)->EnumAllFiles("*.tzod"))
{
	for (auto &fileName: _mapNames)
	{
		fileName.erase(fileName.length() - 5); // remove extension
	}

	std::sort(_mapNames.begin(), _mapNames.end());
}
