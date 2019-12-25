#pragma once
#include <ui/ScrollView.h>
#include <as/MapCollection.h>

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ContentButton;
	class ScanlineLayout;
}

class ShellConfig;
class LangCache;
class MapCollection;

class SelectMapDlg final
	: public UI::ScrollView
	, private MapCollectionListener
{
public:
	SelectMapDlg(FS::FileSystem &fsRoot, WorldView& worldView, ShellConfig &conf, LangCache &lang, MapCollection &mapCollection);

	std::function<void(unsigned int)> eventMapSelected;

private:
	FS::FileSystem& _fs;
	WorldView &_worldView;
	ShellConfig &_conf;
	MapCollection& _mapCollection;
	std::shared_ptr<UI::ScanlineLayout> _mapTiles;

	// MapCollectionListener
	void OnMapAdded(int newMapIndex) override;

	std::shared_ptr<UI::ContentButton> MakeMapTileButton(int mapIndex);
};
