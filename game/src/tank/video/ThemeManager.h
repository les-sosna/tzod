#pragma once

#include <memory>
#include <string>
#include <vector>

class TextureManager;
namespace FS
{
	class FileSystem;
	class MemMap;
}

class ThemeManager
{
public:
	ThemeManager(FS::FileSystem &fs);
	~ThemeManager();
	
	size_t GetThemeCount() const;
	size_t FindTheme(const std::string &name) const;
	std::string GetThemeName(size_t index) const;
	
    bool ApplyTheme(size_t index, TextureManager &tm) const;

private:
	struct ThemeDesc
	{
		std::string fileName;
		std::shared_ptr<FS::MemMap> file;
	};
	
	std::vector<ThemeDesc> _themes;
	FS::FileSystem &_fs;
};
