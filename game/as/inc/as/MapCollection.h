#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <set>

class World;

namespace FS
{
	class FileSystem;
	struct Stream;
}

class MapCollection final
{
public:
	MapCollection(FS::FileSystem &fs);
	~MapCollection();

	int GetMapCount() const { return static_cast<int>(_mapDescs.size()); }
	std::string_view GetMapName(unsigned int mapIndex) const { return _mapDescs[mapIndex].mapName; }
	void AddOrUpdateUserMap(std::string mapName);

	const World& GetCachedWorld(FS::FileSystem& fs, std::string_view mapName);
	std::unique_ptr<World> ExtractCachedWorld(FS::FileSystem& fs, std::string_view mapName);

	std::shared_ptr<FS::Stream> QueryReadStream(FS::FileSystem& fs, const std::string &mapName);


private:
	struct MapFileDesc
	{
		std::string mapName;
		std::unique_ptr<World> cachedWorld;
		bool user;
	};
	std::vector<MapFileDesc> _mapDescs;

	std::set<class MapCollectionListener*> _mapCollectionListeners;
	friend class MapCollectionListener;
};

class MapCollectionListener
{
public:
	explicit MapCollectionListener(MapCollection& mapCollection);
	~MapCollectionListener();
	MapCollection& GetMapCollection() { return _mapCollection; }
	const MapCollection& GetMapCollection() const { return _mapCollection; }

	virtual void OnMapAdded(int newMapIndex) = 0;

private:
	MapCollection& _mapCollection;

	MapCollectionListener(const MapCollectionListener&) = delete;
	void operator=(const MapCollectionListener&) = delete;
};
