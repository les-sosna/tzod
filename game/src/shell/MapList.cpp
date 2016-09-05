#include "MapList.h"
#include "inc/shell/Config.h"

#include <as/AppConstants.h>
#include <fs/FileSystem.h>
#include <MapFile.h>
#include <ui/ConsoleBuffer.h>

#include <sstream>
#include <iomanip>

ListDataSourceMaps::ListDataSourceMaps(FS::FileSystem &fs, UI::ConsoleBuffer &logger)
{
	auto files = fs.GetFileSystem(DIR_MAPS)->EnumAllFiles("*.map");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		std::string tmp = DIR_MAPS;
		tmp += "/";
		tmp += *it;

		try
		{
			MapFile file(*fs.Open(tmp)->QueryStream(), false);
			std::string tmp2 = *it;
			tmp2.erase(it->length() - 4); // cut out the file extension
			int index = AddItem(tmp2);

			std::ostringstream size;
			int h = 0, w = 0;
			file.getMapAttribute("width", w);
			file.getMapAttribute("height", h);
			size << std::setw(3) << w << "*" << std::setw(1) << h;
			SetItemText(index, 1, size.str());

			if( file.getMapAttribute("theme", tmp) )
			{
				SetItemText(index, 2, tmp);
			}
		}
		catch( const std::exception &e )
		{
			logger.Printf(1, "could not open get map attributes - %s", e.what());
		}
	}

	Sort();
}
