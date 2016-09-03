#include "MapCache.h"
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>

static std::unique_ptr<World> LoadMap(FS::FileSystem &fs, const std::string &mapName)
{
	auto stream = fs.GetFileSystem("maps")->Open(mapName + ".map")->QueryStream();

	MapFile file(*stream, false);

	int width, height;
	if (!file.getMapAttribute("width", width) ||
		!file.getMapAttribute("height", height))
	{
		throw std::runtime_error("unknown map size");
	}

	int left = 0;
	int top = 0;
	file.getMapAttribute("west_bound", left);
	file.getMapAttribute("north_bound", top);

	std::unique_ptr<World> world(new World(RectRB{ left, top, left + width, top + height }));
	world->Import(file);
	return world;
}

const World& MapCache::GetCachedWorld(FS::FileSystem &fs, const std::string &mapName)
{
	auto existing = _cachedWorlds.find(mapName);
	if (_cachedWorlds.end() == existing)
	{
		existing = _cachedWorlds.emplace(mapName, LoadMap(fs, mapName)).first;
	}
	return *existing->second;
}

std::unique_ptr<World> MapCache::CheckoutCachedWorld(FS::FileSystem &fs, const std::string &mapName)
{
	auto existing = _cachedWorlds.find(mapName);
	if (_cachedWorlds.end() != existing)
	{
		auto result = std::move(existing->second);
		_cachedWorlds.erase(existing);
		return result;
	}
	else
	{
		return LoadMap(fs, mapName);
	}
}
