#pragma once
#include <ui/Window.h>
#include <gc/World.h>
#include <functional>

class WorldView;

namespace UI
{
	template<class T> struct DataSource;
}

class GameClassVis : public UI::Window
{
public:
	GameClassVis(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView);

	void SetGameClass(std::shared_ptr<UI::DataSource<const std::string&>> className);

	// UI::Window
	void Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	WorldView &_worldView;
	mutable World _world;
	size_t _texSelection;
	std::shared_ptr<UI::DataSource<const std::string&>> _className;
};
