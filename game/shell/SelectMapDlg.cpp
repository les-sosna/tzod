#include "inc/shell/Config.h"
#include "MapPreview.h"
#include "SelectMapDlg.h"
#include <as/MapCollection.h>
#include <loc/Language.h>
#include <render/WorldView.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/ScanlineLayout.h>
#include <ui/ScrollView.h>
#include <ui/StackLayout.h>

SelectMapDlg::SelectMapDlg(FS::FileSystem& fsRoot, WorldView &worldView, ShellConfig &conf, LangCache &lang, MapCollection &mapCollection)
	: MapCollectionListener(mapCollection)
	, _fs(fsRoot)
	, _worldView(worldView)
	, _conf(conf)
	, _mapCollection(mapCollection)
	, _mapTiles(std::make_shared<UI::ScanlineLayout>())
{
	SetContent(_mapTiles);
	_mapTiles->SetElementSize(vec2d{ _conf.ui_tile_size.GetFloat(), _conf.ui_tile_size.GetFloat() });

	OnMapAdded(-1);
}

void SelectMapDlg::OnMapAdded(int newMapIndex)
{
	_mapTiles->UnlinkAllChildren(); // TODO: add just one element

	using namespace UI::DataSourceAliases;
	auto newMapButton = std::make_shared<UI::Button>();
	newMapButton->SetText("+"_txt);
	newMapButton->SetFont("font_default");
	newMapButton->eventClick = [this]()
	{
		eventMapSelected(-1);
	};
	_mapTiles->AddFront(newMapButton);

	auto mapCount = GetMapCollection().GetMapCount();
	for (int mapIndex = 0; mapIndex < mapCount; mapIndex++)
	{
		_mapTiles->AddFront(MakeMapTileButton(mapIndex));
	}

	_mapTiles->SetFocus(&_mapTiles->GetChild(0));
}

std::shared_ptr<UI::ContentButton> SelectMapDlg::MakeMapTileButton(int mapIndex)
{
	auto mapPreview = std::make_shared<MapPreview>(_fs, _worldView, _mapCollection);
	mapPreview->Resize(_conf.ui_tile_size.GetFloat(), _conf.ui_tile_size.GetFloat());
	mapPreview->SetPadding(_conf.ui_tile_spacing.GetFloat() / 2);
	mapPreview->SetMapName(std::make_shared<UI::StaticText>(GetMapCollection().GetMapName(mapIndex)));

	auto mpButton = std::make_shared<UI::ContentButton>();
	mpButton->SetContent(mapPreview);
	mpButton->Resize(_conf.ui_tile_size.GetFloat(), _conf.ui_tile_size.GetFloat());
	mpButton->eventClick = [this, mapIndex]()
	{
		eventMapSelected(mapIndex);
	};

	return mpButton;
}
