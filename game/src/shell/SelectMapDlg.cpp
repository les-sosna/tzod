#include "inc/shell/Config.h"
#include "MapPreview.h"
#include "SelectMapDlg.h"
#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <render/WorldView.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/ScanlineLayout.h>
#include <ui/StackLayout.h>

SelectMapDlg::SelectMapDlg(WorldView &worldView, FS::FileSystem &fsRoot, ShellConfig &conf, LangCache &lang, MapCache &mapCache)
	: _worldView(worldView)
	, _conf(conf)
	, _lang(lang)
	, _mapCache(mapCache)
	, _mapTiles(std::make_shared<UI::ScanlineLayout>())
{
	Resize(800, 600);

	auto rootLayout = std::make_shared<UI::StackLayout>();
	AddFront(rootLayout);

	rootLayout->AddFront(_mapTiles);

	auto mapPreview = std::make_shared<MapPreview>(fsRoot, _worldView, _mapCache);
	mapPreview->Resize(_conf.tile_size.GetFloat(), _conf.tile_size.GetFloat());
	mapPreview->SetPadding(_conf.tile_spacing.GetFloat() / 2);
	mapPreview->SetMapName(std::make_shared<UI::StaticText>("dm1"));

	auto mpButton = std::make_shared<UI::ButtonBase>();
	mpButton->AddFront(mapPreview);
	mpButton->Resize(_conf.tile_size.GetFloat(), _conf.tile_size.GetFloat());
//	mpButton->eventClick = std::bind(&SinglePlayer::OnOK, this, (int)mapIndex);

	_mapTiles->AddFront(mpButton);
	_mapTiles->AddFront(mpButton);
	_mapTiles->AddFront(mpButton);
	_mapTiles->AddFront(mpButton);
}
