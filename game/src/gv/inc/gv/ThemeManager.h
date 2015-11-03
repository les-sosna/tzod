#pragma once

#include <app/AppStateListener.h>
#include <memory>
#include <string>
#include <vector>

class TextureManager;
namespace FS
{
	class FileSystem;
	class MemMap;
}

class ThemeManager : private AppStateListener
{
public:
	ThemeManager(AppState &appState, FS::FileSystem &fs, TextureManager &tm);
	~ThemeManager();

	size_t GetThemeCount() const;
	std::string GetThemeName(size_t index) const;

private:
	struct ThemeDesc
	{
		std::string fileName;
		std::shared_ptr<FS::MemMap> file;
	};

	std::vector<ThemeDesc> _themes;
	FS::FileSystem &_fs;
	TextureManager &_textureManager;

	// AppStateListener
	void OnGameContextChanging() override;
	void OnGameContextChanged() override;

	ThemeManager(const ThemeManager&) = delete;
};
