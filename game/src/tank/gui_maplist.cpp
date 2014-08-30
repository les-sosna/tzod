// gui_maplist.cpp

#include "gui_maplist.h"

#include "constants.h"
#include "globals.h"
#include "MapFile.h"

#include "config/Config.h"
#include "core/debug.h"

#include <FileSystem.h>

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

ListDataSourceMaps::ListDataSourceMaps(FS::FileSystem &fs)
{
	auto files = fs.GetFileSystem(DIR_MAPS)->EnumAllFiles("*.map");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		std::string tmp = DIR_MAPS;
		tmp += "/";
		tmp += *it;

		try
		{
			MapFile file(fs.Open(tmp)->QueryStream(), false);
			std::string tmp2 = *it;
			tmp2.erase(it->length() - 4); // cut out the file extension
			int index = AddItem(tmp2);

			char size[64];
			int h = 0, w = 0;
			file.getMapAttribute("width", w);
			file.getMapAttribute("height", h);
			sprintf(size, "%3d*%d", w, h);
			SetItemText(index, 1, size);

			if( file.getMapAttribute("theme", tmp) )
			{
				SetItemText(index, 2, tmp);
			}
		}
		catch( const std::exception &e )
		{
			TRACE("could not open get map attributes - %s", e.what());
		}
	}

	Sort();
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file



