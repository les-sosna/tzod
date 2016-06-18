#pragma once
#include <ui/Dialog.h>
#include <thread>

class ConfCache;
class World;
class WorldView;
namespace FS
{
	class FileSystem;
}

class SinglePlayer : public UI::Dialog
{
public:
	SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs, ConfCache &conf);

private:
	void OnClickMap(std::string mapName);
	ConfCache &_conf;
};
