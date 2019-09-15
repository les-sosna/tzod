#include "inc/shell/detail/MapCollection.h"
#include <fs/FileSystem.h>

MapCollection::MapCollection(FS::FileSystem &fs)
	: _mapNames(fs.EnumAllFiles("*.tzod"))
{
	for (auto &fileName: _mapNames)
	{
		fileName.erase(fileName.length() - 5); // remove .map extension
	}
}
