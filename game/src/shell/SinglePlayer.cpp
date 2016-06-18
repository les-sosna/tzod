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
	: UI::Dialog(manager, texman, 562, 400)
	, _conf(conf)
{
	std::vector<std::string> maps = { "dm1", "skew-dm", "dm3", "dm5" };
	float x = 10;
	for( auto map: maps )
	{
		auto mp = std::make_shared<MapPreview>(manager, texman, worldView);
		mp->Resize(128, 128);
		mp->Move(x, 10.f);
		mp->SetMapName(map, fs);
		mp->eventClick = [this, map] {OnClickMap(map);};
		x += 128.f + 10.f;
		AddFront(mp);
	}
}

void SinglePlayer::OnClickMap(std::string mapName)
{
	_conf.cl_map.Set(mapName);
	_conf.ui_showmsg.Set(true);
	Close(_resultOK);
}
