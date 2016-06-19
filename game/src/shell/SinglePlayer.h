#pragma once
#include <ui/Dialog.h>
#include <array>
#include <memory>

class MapPreview;
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

	// UI::Window
	void OnSize(float width, float height) override;

private:
	void OnClickMap(std::string mapName);
	ConfCache &_conf;
	std::array<std::shared_ptr<MapPreview>, 4> _tiles;
};
