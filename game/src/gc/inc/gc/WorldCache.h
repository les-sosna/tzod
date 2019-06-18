#pragma once
#include <map>
#include <memory>
#include <string>
#include <string_view>

class World;
namespace FS
{
	class FileSystem;
}

class WorldCache final
{
public:
	const World& GetCachedWorld(FS::FileSystem &fs, std::string_view mapName);
	std::unique_ptr<World> CheckoutCachedWorld(FS::FileSystem &fs, std::string_view mapName);

private:
	std::map<std::string, std::unique_ptr<World>, std::less<>> _cachedWorlds;
};

std::unique_ptr<World> LoadMapUncached(FS::FileSystem &fs, std::string_view mapName);
