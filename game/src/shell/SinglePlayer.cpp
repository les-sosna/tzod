#include "SinglePlayer.h"
#include "BotView.h"
#include "MapPreview.h"
#include "inc/shell/Config.h"
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <render/WorldView.h>
#include <video/DrawingContext.h>
#include <ui/StackLayout.h>
#include <ui/Text.h>
#include <ui/DataSource.h>

static const float c_tileSize = 180;
static const float c_tileSpacing = 16;

using namespace UI::DataSourceAliases;

SinglePlayer::SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs, ConfCache &conf)
	: UI::Dialog(manager, texman, 1, 1)
	, _conf(conf)
	, _tierTitle(std::make_shared<UI::Text>(manager, texman))
	, _enemiesTitle(std::make_shared<UI::Text>(manager, texman))
{
	_tierTitle->SetAlign(alignTextCT);
	_tierTitle->SetFont(texman, "font_default");
	_tierTitle->SetText("Tier 1"_txt);
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
	_enemiesTitle->SetText("Enemies"_txt);
	AddFront(_enemiesTitle);

	_enemies = std::make_shared<UI::StackLayout>(manager);
	_enemies->SetFlowDirection(UI::FlowDirection::Horizontal);
	AddFront(_enemies);

	for (size_t i = 0; i < _conf.dm_bots.GetSize(); ++i)
	{
		auto botView = std::make_shared<BotView>(manager, texman);
		botView->SetBotConfig(_conf.dm_bots.GetAt(i).AsTable(), texman);
		botView->Resize(64, 64);
		_enemies->AddFront(botView);
	}
}

void SinglePlayer::OnClickMap(std::string mapName)
{
	_conf.cl_map.Set(mapName);
	Close(_resultOK);
}

void SinglePlayer::OnSize(float width, float height)
{
	const size_t columns = 4;
	const size_t rows = (_tiles.size() + columns - 1) / columns;

	float y = c_tileSpacing;

	_tierTitle->Move(width / 2, y);
	y += _tierTitle->GetHeight();

	y += c_tileSpacing;

	float x = (width - (c_tileSize + c_tileSpacing) * (float)columns + c_tileSpacing) / 2;
	for (size_t i = 0; i < _tiles.size(); i++)
	{
		_tiles[i]->Move(x + (float)(i % columns) * (c_tileSize + c_tileSpacing), y + (float)(i / columns) * (c_tileSize + c_tileSpacing));
		_tiles[i]->Resize(c_tileSize, c_tileSize);
	}
	y += (float)rows * (c_tileSize + c_tileSpacing);

	y += c_tileSpacing;

	_enemiesTitle->Move(x, y);
	y += _enemiesTitle->GetHeight();

	y += c_tileSpacing;

	_enemies->Move(x, y);
	_enemies->Resize(width - c_tileSpacing / 2, 64);
}

