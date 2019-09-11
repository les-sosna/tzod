#include "inc/shell/detail/MapCollection.h"
#include <fs/FileSystem.h>

MapCollection::MapCollection(FS::FileSystem &fs)
	: _mapNames(fs.EnumAllFiles("*.map"))
{
	for (auto &fileName: _mapNames)
	{
		fileName.erase(fileName.length() - 4); // remove .map extension
	}
}
