// gui_maplist.cpp

#include "stdafx.h"

#include "gui_maplist.h"

#include "fs/FileSystem.h"
#include "fs/MapFile.h"

#include "config/Config.h"

#include "core/debug.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

ListDataSourceMaps::ListDataSourceMaps()
{
	std::set<string_t> files;
	g_fs->GetFileSystem(DIR_MAPS)->EnumAllFiles(files, TEXT("*.map"));
	for( std::set<string_t>::iterator it = files.begin(); it != files.end(); ++it )
	{
		string_t tmp = DIR_MAPS;
		tmp += "/";
		tmp += *it;

		try
		{
			MapFile file(g_fs->Open(tmp)->QueryStream(), false);
			string_t tmp2 = *it;
			tmp2.erase(it->length() - 4); // cut out the file extension
			int index = AddItem(tmp2);

			char size[64];
			int h = 0, w = 0;
			file.getMapAttribute("width", w);
			file.getMapAttribute("height", h);
			wsprintf(size, "%3d*%d", w, h);
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



