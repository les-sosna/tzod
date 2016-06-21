#include "SinglePlayer.h"
#include "BotView.h"
#include "MapPreview.h"
#include "inc/shell/Config.h"
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <render/WorldView.h>
#include <video/DrawingContext.h>
#include <ui/Text.h>

SinglePlayer::SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs, ConfCache &conf)
	: UI::Dialog(manager, texman, 1, 1)
	, _conf(conf)
	, _tierTitle(std::make_shared<UI::Text>(manager, texman))
	, _enemiesTitle(std::make_shared<UI::Text>(manager, texman))
{
	_tierTitle->SetAlign(alignTextCT);
	_tierTitle->SetFont(texman, "font_default");
	_tierTitle->SetText(texman, "Tier 1");
	AddFront(_tierTitle);

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

	_enemiesTitle->SetFont(texman, "font_default");
	_enemiesTitle->SetText(texman, "Enemies");
	AddFront(_enemiesTitle);

	for (size_t i = 0; i < _conf.dm_bots.GetSize(); ++i)
	{
		auto botView = std::make_shared<BotView>(manager, texman);
		botView->SetBotConfig(_conf.dm_bots.GetAt(i).AsTable(), texman);
		botView->Resize(64, 64);
		AddFront(botView);
		_enemies.push_back(botView);
	}
}

void SinglePlayer::OnClickMap(std::string mapName)
{
	_conf.cl_map.Set(mapName);
	Close(_resultOK);
}

void SinglePlayer::OnSize(float width, float height)
{
	const float tileSize = 180;
	const float tileSpacing = 16;
	const size_t columns = 4;
	const size_t rows = (_tiles.size() + columns - 1) / columns;

	float y = tileSpacing;

	_tierTitle->Move(width / 2, y);
	y += _tierTitle->GetHeight();

	y += tileSpacing;

	float x = (width - (tileSize + tileSpacing) * (float)columns + tileSpacing) / 2;
	for (size_t i = 0; i < _tiles.size(); i++)
	{
		_tiles[i]->Move(x + (float)(i % columns) * (tileSize + tileSpacing), y + (float)(i / columns) * (tileSize + tileSpacing));
		_tiles[i]->Resize(tileSize, tileSize);
	}
	y += (float)rows * (tileSize + tileSpacing);

	y += tileSpacing;

	_enemiesTitle->Move(x, y);
	y += _enemiesTitle->GetHeight();

	y += tileSpacing;

	for (size_t i = 0; i < _enemies.size(); i++)
	{
		_enemies[i]->Move(x + (_enemies[i]->GetWidth() + tileSpacing) * (float)i, y);
	}
}

