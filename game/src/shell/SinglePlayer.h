#pragma once
#include <ui/Dialog.h>
#include <array>
#include <memory>
#include <vector>

class BotView;
class MapPreview;
class ConfCache;
class World;
class WorldView;
namespace FS
{
	class FileSystem;
}
namespace UI
{
	class Text;
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
	std::shared_ptr<UI::Text> _tierTitle;
	std::shared_ptr<UI::Text> _enemiesTitle;
	std::vector<std::shared_ptr<BotView>> _enemies;
};
