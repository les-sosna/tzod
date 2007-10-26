// gui_maplist.cpp

#include "stdafx.h"

#include "gui_maplist.h"

#include "fs/FileSystem.h"
#include "fs/MapFile.h"

#include "config/Config.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

MapList::MapList(Window *parent, float x, float y, float width, float height)
  : List(parent, x, y, width, height)
{
	SetTabPos(0,   4); // name
	SetTabPos(1, 384); // size
	SetTabPos(2, 448); // theme

	SafePtr<IFileSystem> dir = g_fs->GetFileSystem(DIR_MAPS);
	if( dir )
	{
		std::set<string_t> files;
		if( dir->EnumAllFiles(files, TEXT("*.map")) )
		{
			int lastMapIndex = 0;

			for( std::set<string_t>::iterator it = files.begin(); it != files.end(); ++it )
			{
				string_t tmp = DIR_MAPS;
				tmp += "/";
				tmp += *it;

				MapFile file;
				if( file.Open(tmp.c_str(), false) )
				{
					it->erase(it->length() - 4); // cut out the file extension
					int index = AddItem(it->c_str());

					if( *it == g_conf.cl_map->Get() )
						lastMapIndex = index;

					char size[64];
					int h = 0, w = 0;
					file.getMapAttribute("width", w);
					file.getMapAttribute("height", h);
					wsprintf(size, "%3d*%d", w, h);
					SetItemText(index, 1, size);

					if( file.getMapAttribute("theme", tmp) )
					{
						SetItemText(index, 2, tmp.c_str());
					}
				}
			}

			SetCurSel(lastMapIndex, false);
			ScrollTo(lastMapIndex - (GetNumLinesVisible() - 1) * 0.5f);
		}
		else
		{
			_ASSERT(FALSE); // EnumAllFiles has returned error...
		}
	}
}

MapList::~MapList()
{
}



///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file



