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
	
	typedef std::vector<std::unique_ptr<ObjectView>> ViewCollection;
	std::vector<ViewCollection> _type2views;
	template <class T, class View, class ...Args>
	void AddView(Args && ...args)
	{
		auto type = T::GetTypeStatic();
		if (_type2views.size() <= type)
			_type2views.resize(type + 1);
		_type2views[type].emplace_back(new View(std::forward<Args>(args)...));
	}
	const ViewCollection* GetViews(const GC_Actor &actor) const;
};
