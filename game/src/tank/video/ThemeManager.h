#pragma once

#include "core/singleton.h"

#include <memory>
#include <string>
#include <vector>

namespace FS
{
	class MemMap;
}

class ThemeManager
{
	struct ThemeDesc
	{
		std::string fileName;
		std::shared_ptr<FS::MemMap> file;
	};
	
	std::vector<ThemeDesc> _themes;
	
public:
	ThemeManager();
	~ThemeManager();
	
	size_t GetThemeCount();
	size_t FindTheme(const std::string &name);
	std::string GetThemeName(size_t index);
	
    bool ApplyTheme(size_t index);
};

typedef StaticSingleton<ThemeManager> _ThemeManager;
