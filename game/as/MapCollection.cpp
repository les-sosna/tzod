#include "inc/as/MapCollection.h"
#include "inc/as/AppConstants.h"
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <MapFile.h>
#include <algorithm>
#include <cassert>

static constexpr auto CompareMapName = [](std::string_view a, std::string_view b)
{
	if (a.front() == '!' && b.front() == '!')
		return a > b;
	else
		return a < b;
};


MapCollection::MapCollection(FS::FileSystem &fs)
{
	std::map<std::string, bool, decltype(CompareMapName)> isUserMap(CompareMapName);

	for (auto &fileName: fs.GetFileSystem(DIR_MAPS)->EnumAllFiles("*.tzod"))
	{
		fileName.erase(fileName.length() - 5); // remove extension
		isUserMap.emplace(std::move(fileName), false);
	}

	for (auto& fileName: fs.GetFileSystem("user")->GetFileSystem(DIR_MAPS, true /* create */)->EnumAllFiles("*.tzod"))
	{
		fileName.erase(fileName.length() - 5); // remove extension
		isUserMap[std::move(fileName)] = true;
	}

	_mapDescs.reserve(isUserMap.size());
	for (auto &m2u: isUserMap)
	{
		_mapDescs.push_back({ m2u.first, nullptr, m2u.second });
	}
}

MapCollection::~MapCollection()
{
	assert(_mapCollectionListeners.empty());
}

static constexpr auto CompareDesc = [](auto& desc, std::string_view mapName)
{
	return CompareMapName(desc.mapName, mapName);
};

void MapCollection::AddOrUpdateUserMap(std::string mapName)
{
	auto insertAt = std::lower_bound(_mapDescs.begin(), _mapDescs.end(), mapName, CompareDesc);

	if (insertAt != _mapDescs.end() && insertAt->mapName == mapName)
	{
		// override existing
		insertAt->user = true;
		insertAt->cachedWorld.reset();
	}
	else
	{
		auto newMapIndex = static_cast<int>(std::distance(_mapDescs.begin(), insertAt));
		_mapDescs.insert(insertAt, { std::move(mapName), nullptr, true });
		for (auto* ls : _mapCollectionListeners)
			ls->OnMapAdded(newMapIndex);
	}
}

static std::unique_ptr<World> LoadMapUncached(FS::FileSystem& fs, std::string_view mapName)
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

const World& MapCollection::GetCachedWorld(FS::FileSystem& fs, std::string_view mapName)
{
	auto it = std::lower_bound(_mapDescs.begin(), _mapDescs.end(), mapName, CompareDesc);
	assert(it != _mapDescs.end() && it->mapName == mapName);

	if (!it->cachedWorld)
		it->cachedWorld = LoadMapUncached(*(it->user ? *fs.GetFileSystem("user") : fs).GetFileSystem(DIR_MAPS, true), mapName);

	return *it->cachedWorld;
}

std::unique_ptr<World> MapCollection::ExtractCachedWorld(FS::FileSystem& fs, std::string_view mapName)
{
	auto it = std::lower_bound(_mapDescs.begin(), _mapDescs.end(), mapName, CompareDesc);
	assert(it != _mapDescs.end() && it->mapName == mapName);

	if (it->cachedWorld)
	{
		return std::move(it->cachedWorld);
	}
	else
	{
		return LoadMapUncached(*(it->user ? *fs.GetFileSystem("user") : fs).GetFileSystem(DIR_MAPS, true), mapName);
	}
}

std::shared_ptr<FS::Stream> MapCollection::QueryReadStream(FS::FileSystem& fs, const std::string &mapName)
{
	auto it = std::lower_bound(_mapDescs.begin(), _mapDescs.end(), mapName, CompareDesc);
	assert(it != _mapDescs.end() && it->mapName == mapName);

	auto mapsFolder = (it->user ? *fs.GetFileSystem("user") : fs).GetFileSystem(DIR_MAPS, true);
	return mapsFolder->Open(mapName + ".tzod")->QueryStream();
}

MapCollectionListener::MapCollectionListener(MapCollection& mapCollection)
	: _mapCollection(mapCollection)
{
	_mapCollection._mapCollectionListeners.insert(this);
}

MapCollectionListener::~MapCollectionListener()
{
	_mapCollection._mapCollectionListeners.erase(this);
}
