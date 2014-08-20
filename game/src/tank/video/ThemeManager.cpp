#include "ThemeManager.h"
#include "TextureManager.h"
#include "constants.h"
#include "globals.h"

#include <fs/FileSystem.h>

ThemeManager::ThemeManager()
{
	std::shared_ptr<FS::FileSystem> dir = g_fs->GetFileSystem(DIR_THEMES);
	auto files = dir->EnumAllFiles("*.lua");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		ThemeDesc td;
		td.fileName = std::move(*it);
		td.file = dir->Open(td.fileName)->QueryMap();
		_themes.push_back(td);
	}
}

ThemeManager::~ThemeManager()
{
}

size_t ThemeManager::GetThemeCount()
{
	return _themes.size() + 1;
}

size_t ThemeManager::FindTheme(const std::string &name)
{
	for( size_t i = 0; i < _themes.size(); i++ )
	{
		if( GetThemeName(i+1) == name )
		{
			return i+1;
		}
	}
	return 0;
}

std::string ThemeManager::GetThemeName(size_t index)
{
	if( 0 == index )
		return "<standard>";
	return _themes[index-1].fileName.substr(
											0, _themes[index-1].fileName.size() - 4); // throw off the extension
}

bool ThemeManager::ApplyTheme(size_t index)
{
	bool res = (g_texman->LoadPackage(FILE_TEXTURES, g_fs->Open(FILE_TEXTURES)->QueryMap()) > 0);
	if( index > 0 )
	{
		res = res && (g_texman->LoadPackage(_themes[index-1].fileName, _themes[index-1].file) > 0);
	}
	return res;
}
