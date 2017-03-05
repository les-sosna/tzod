#pragma once
#include <map>
#include <memory>
#include <string>

class World;
namespace FS
{
	class FileSystem;
}

class MapCache
{
public:
	const World& GetCachedWorld(FS::FileSystem &fs, const std::string &mapName);
	std::unique_ptr<World> CheckoutCachedWorld(FS::FileSystem &fs, const std::string &mapName);

private:
	std::map<std::string, std::unique_ptr<World>> _cachedWorlds;
};
