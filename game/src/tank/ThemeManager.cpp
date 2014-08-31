#include "constants.h"
#include "ThemeManager.h"
#include <fs/FileSystem.h>
#include <video/TextureManager.h>

ThemeManager::ThemeManager(FS::FileSystem &fs)
	: _fs(fs)
{
	std::shared_ptr<FS::FileSystem> dir = _fs.GetFileSystem(DIR_THEMES);
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

size_t ThemeManager::GetThemeCount() const
{
	return _themes.size() + 1;
}

size_t ThemeManager::FindTheme(const std::string &name) const
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

std::string ThemeManager::GetThemeName(size_t index) const
{
	if( 0 == index )
		return "<standard>";
	return _themes[index-1].fileName.substr(0, _themes[index-1].fileName.size() - 4); // throw off the extension
}

bool ThemeManager::ApplyTheme(size_t index, TextureManager &tm) const
{
	bool res = (tm.LoadPackage(FILE_TEXTURES, _fs.Open(FILE_TEXTURES)->QueryMap(), _fs) > 0);
	if( index > 0 )
	{
		res = res && (tm.LoadPackage(_themes[index-1].fileName, _themes[index-1].file, _fs) > 0);
	}
	return res;
}
