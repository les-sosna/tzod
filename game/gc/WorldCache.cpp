#include "inc/gc/World.h"
#include "inc/gc/WorldCache.h"
#include <MapFile.h>
#include <fs/FileSystem.h>

std::unique_ptr<World> LoadMapUncached(FS::FileSystem &fs, std::string_view mapName)
{
	auto stream = fs.Open(std::string(mapName) + ".tzod")->QueryStream();

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

	std::unique_ptr<World> world(new World(RectRB{ left, top, left + width, top + height }, true /* initField */));
	world->Import(file);
	return world;
}

const World& WorldCache::GetCachedWorld(FS::FileSystem &fs, std::string_view mapName)
{
	auto existing = _cachedWorlds.find(mapName);
	if (_cachedWorlds.end() == existing)
	{
		existing = _cachedWorlds.emplace(mapName, LoadMapUncached(*fs.GetFileSystem("maps"), mapName)).first;
	}
	return *existing->second;
}

std::unique_ptr<World> WorldCache::CheckoutCachedWorld(FS::FileSystem &fs, std::string_view mapName)
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
		return LoadMapUncached(*fs.GetFileSystem("maps"), mapName);
	}
}
