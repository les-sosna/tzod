#include "inc/shell/detail/MapCollection.h"
#include "inc/shell/Config.h"
#include "MapPreview.h"
#include "SelectMapDlg.h"
#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <render/WorldView.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/ScanlineLayout.h>
#include <ui/ScrollView.h>
#include <ui/StackLayout.h>

SelectMapDlg::SelectMapDlg(WorldView &worldView, FS::FileSystem &fsRoot, ShellConfig &conf, LangCache &lang, WorldCache &worldCache, MapCollection &mapCollection)
	: _worldView(worldView)
	, _conf(conf)
	, _worldCache(worldCache)
	, _mapCollection(mapCollection)
	, _mapTiles(std::make_shared<UI::ScanlineLayout>())
{
	SetContent(_mapTiles);
	_mapTiles->SetElementSize(vec2d{ _conf.ui_tile_size.GetFloat(), _conf.ui_tile_size.GetFloat() });

	for (unsigned int mapIndex = 0; mapIndex < _mapCollection.GetMapCount(); mapIndex++)
	{
		auto mapPreview = std::make_shared<MapPreview>(fsRoot, _worldView, _worldCache);
		mapPreview->Resize(_conf.ui_tile_size.GetFloat(), _conf.ui_tile_size.GetFloat());
		mapPreview->SetPadding(_conf.ui_tile_spacing.GetFloat() / 2);
		mapPreview->SetMapName(std::make_shared<UI::StaticText>(_mapCollection.GetMapName(mapIndex)));

		auto mpButton = std::make_shared<UI::ButtonBase>();
		mpButton->AddFront(mapPreview);
		mpButton->Resize(_conf.ui_tile_size.GetFloat(), _conf.ui_tile_size.GetFloat());
		mpButton->eventClick = [this, mapIndex]()
		{
			eventMapSelected(std::static_pointer_cast<SelectMapDlg>(shared_from_this()), mapIndex);
		};

		_mapTiles->AddFront(mpButton);
	}

	using namespace UI::DataSourceAliases;
	auto newMapButton = std::make_shared<UI::Button>();
	newMapButton->SetText("+"_txt);
	newMapButton->SetFont("font_default");
	newMapButton->eventClick = [this]()
	{
		eventMapSelected(std::static_pointer_cast<SelectMapDlg>(shared_from_this()), -1);
	};
	_mapTiles->AddFront(newMapButton);

	_mapTiles->SetFocus(_mapTiles->GetChild(0));
}
