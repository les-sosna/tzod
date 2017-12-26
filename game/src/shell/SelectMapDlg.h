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
class MapCache;
class WorldView;

class SelectMapDlg : public UI::Dialog
{
public:
	SelectMapDlg(WorldView &worldView, FS::FileSystem &fsRoot, ShellConfig &conf, LangCache &lang, MapCache &mapCache);

private:
	WorldView &_worldView;
	ShellConfig &_conf;
	LangCache &_lang;
	MapCache &_mapCache;
	std::shared_ptr<UI::ScanlineLayout> _mapTiles;
};
