#include "SinglePlayer.h"
#include "MapPreview.h"
#include "inc/shell/Config.h"
#include <render/WorldView.h>
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <video/DrawingContext.h>
#include <thread>

SinglePlayer::SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs, ConfCache &conf)
	: UI::Dialog(manager, texman, 1, 1)
	, _conf(conf)
{
	std::vector<std::string> maps = { "dm1", "dm2", "dm3", "dm4" };
	int index = 0;
	for( auto map: maps )
	{
		auto mp = std::make_shared<MapPreview>(manager, texman, worldView);
		mp->SetMapName(map, fs);
		mp->eventClick = [this, map] {OnClickMap(map);};
		AddFront(mp);
		_tiles[index] = mp;
		index++;
	}
}

void SinglePlayer::OnClickMap(std::string mapName)
{
	_conf.cl_map.Set(mapName);
	Close(_resultOK);
}

void SinglePlayer::OnSize(float width, float height)
{
	const float tileSize = 256;
	const float tileSpacing = 16;
	const size_t columns = 2;
	const size_t rows = (_tiles.size() + columns - 1) / columns;
	float x = (width - tileSize * (float)columns - tileSpacing) / 2;
	float y = (height - tileSize * (float)rows - tileSpacing) / 2;
	for (size_t i = 0; i < _tiles.size(); i++)
	{
		_tiles[i]->Move(x + (float)(i % columns) * (tileSize + tileSpacing), y + (float)(i / columns) * (tileSize + tileSpacing));
		_tiles[i]->Resize(tileSize, tileSize);
	}
}

