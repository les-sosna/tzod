#pragma once
#include <math/MyMath.h>
#include <vector>
#include <string>
#include <memory>

namespace FS
{
	struct MemMap;
	class FileSystem;
}

struct PackageSpriteDesc
{
	std::string textureFilePath;
	std::string spriteName;

	// mandatory
	int xframes;
	int yframes;
	int leadChar;
	float border;
	vec2d scale;
	vec2d atlasOffset;

	// optional - default values are relative to image size cannot be resolved until image is loaded
	vec2d atlasSize;
	vec2d pivot;

	// flags
	bool magFilter : 1;
	bool wrappable : 1;
	bool hasPivotX : 1;
	bool hasPivotY : 1;
	bool hasSizeX : 1;
	bool hasSizeY : 1;
};

std::vector<PackageSpriteDesc> ParsePackage(const std::string& packageName, std::shared_ptr<FS::MemMap> file, FS::FileSystem& fs);
std::vector<PackageSpriteDesc> ParseDirectory(const std::string& dirName, const std::string& texPrefix, FS::FileSystem& fs, bool magFilter);
