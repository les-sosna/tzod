#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace FS
{
	class FileSystem;
}

class MapCollection final
{
public:
	MapCollection(FS::FileSystem &fs);

	unsigned int GetMapCount() const { return static_cast<unsigned int>(_mapNames.size()); }
	std::string_view GetMapName(unsigned int mapIndex) const { return _mapNames[mapIndex]; }

private:
	std::vector<std::string> _mapNames;
};
