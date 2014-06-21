#pragma once

#include "Terrain.h"
#include <cstddef>
#include <memory>
#include <vector>

class TextureManager;
class World;
class GC_Actor;
struct FRECT;
struct IRender;
struct ObjectView;

class WorldView
{
public:
    WorldView(IRender &render, TextureManager &texman);
	~WorldView();
	void Render(World &world, const FRECT &view, bool editorMode) const;

private:
    IRender &_render;
    TextureManager &_tm;
	Terrain _terrain;
	
	std::vector<std::unique_ptr<ObjectView>> _type2view;
	template <class T, class View, class ...Args>
	void AddView(Args && ...args)
	{
		auto type = T::GetTypeStatic();
		if (_type2view.size() <= type)
			_type2view.resize(type + 1);
		_type2view[type].reset(new View(std::forward<Args>(args)...));
	}
	ObjectView* GetView(const GC_Actor &actor) const;
};
