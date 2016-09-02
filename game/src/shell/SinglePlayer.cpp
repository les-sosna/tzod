#include "SinglePlayer.h"
#include "BotView.h"
#include "MapPreview.h"
#include "inc/shell/Config.h"
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <render/WorldView.h>
#include <video/DrawingContext.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/StackLayout.h>
#include <ui/Text.h>

static const float c_tileSize = 180;
static const float c_tileSpacing = 16;

using namespace UI::DataSourceAliases;

SinglePlayer::SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs, ConfCache &conf)
	: UI::Dialog(manager, texman)
	, _conf(conf)
	, _content(std::make_shared<UI::StackLayout>(manager))
	, _tierTitle(std::make_shared<UI::Text>(manager, texman))
	, _tiles(std::make_shared<UI::StackLayout>(manager))
	, _enemiesTitle(std::make_shared<UI::Text>(manager, texman))
{
	_tierTitle->SetFont(texman, "font_default");
	_tierTitle->SetText("Tier 1"_txt);
	_content->AddFront(_tierTitle);

	std::vector<std::string> maps = { "dm1", "dm2", "dm3", "dm4" };
	for( auto map: maps )
	{
		auto mp = std::make_shared<MapPreview>(manager, texman, worldView);
		mp->Resize(c_tileSize, c_tileSize);
		mp->SetMapName(map, fs);
		mp->eventClick = [this, map] {OnClickMap(map);};
		_tiles->AddFront(mp);
	}
	_tiles->SetFlowDirection(UI::FlowDirection::Horizontal);
	_tiles->SetSpacing(c_tileSpacing);
	_content->AddFront(_tiles);

	_enemiesTitle->SetFont(texman, "font_default");
	_enemiesTitle->SetText("Enemies"_txt);
	_content->AddFront(_enemiesTitle);

	_enemies = std::make_shared<UI::StackLayout>(manager);
	_enemies->SetFlowDirection(UI::FlowDirection::Horizontal);
	_content->AddFront(_enemies);

	for (size_t i = 0; i < _conf.dm_bots.GetSize(); ++i)
	{
		auto botView = std::make_shared<BotView>(manager, texman);
		botView->SetBotConfig(_conf.dm_bots.GetAt(i).AsTable(), texman);
		botView->Resize(64, 64);
		_enemies->AddFront(botView);
	}

	_content->SetSpacing(c_tileSpacing);
	AddFront(_content);
}

void SinglePlayer::OnClickMap(std::string mapName)
{
	_conf.cl_map.Set(mapName);
	Close(_resultOK);
}

FRECT SinglePlayer::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const
{
	if (_content.get() == &child)
	{
		float pxMargin = std::floor(c_tileSpacing * lc.GetScale());
		vec2d pxMargins = { pxMargin, pxMargin };
		return MakeRectRB(pxMargins, lc.GetPixelSize() - pxMargins);
	}

	return UI::Dialog::GetChildRect(texman, lc, sc, child);
}
