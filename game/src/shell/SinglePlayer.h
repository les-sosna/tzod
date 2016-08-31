#pragma once
#include <ui/Dialog.h>
#include <array>
#include <memory>
#include <vector>

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
	class StackLayout;
	class Text;
}

class SinglePlayer : public UI::Dialog
{
public:
	SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs, ConfCache &conf);

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const override;

private:
	void OnClickMap(std::string mapName);
	ConfCache &_conf;
	std::shared_ptr<UI::StackLayout> _content;

	std::shared_ptr<UI::Text> _tierTitle;
	std::shared_ptr<UI::StackLayout> _tiles;
	std::shared_ptr<UI::Text> _enemiesTitle;
	std::shared_ptr<UI::StackLayout> _enemies;
};
