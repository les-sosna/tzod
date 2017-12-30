#pragma once
#include <ui/Dialog.h>

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ScanlineLayout;
}

class ShellConfig;
class LangCache;
class MapCollection;
class WorldCache;
class WorldView;

class SelectMapDlg : public UI::Dialog
{
public:
	SelectMapDlg(WorldView &worldView, FS::FileSystem &fsRoot, ShellConfig &conf, LangCache &lang, WorldCache &worldCache, MapCollection &mapCollection);

	// Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const Window &child) const override;

private:
	WorldView &_worldView;
	ShellConfig &_conf;
	LangCache &_lang;
	WorldCache &_worldCache;
	MapCollection &_mapCollection;
	std::shared_ptr<UI::ScanlineLayout> _mapTiles;
};
