// TextureManager.h

#pragma once

#include "RenderBase.h"

struct LogicalTexture
{
	DEV_TEXTURE dev_texture;

	float left;
	float top;
	float right;
	float bottom;
	float pixel_width;
	float pixel_height;
	float frame_width;
	float frame_height;
	float pivot_x;
	float pivot_y;
	int   xframes;
	int   yframes;
	SpriteColor color;
};

class TextureManager
{
	struct TexDesc
	{
		DEV_TEXTURE id;
		int width;          // The Width Of The Entire Image.
		int height;         // The Height Of The Entire Image.
		int refCount;       // number of logical textures
	};
	typedef std::list<TexDesc>       TexDescList;
	typedef TexDescList::iterator    TexDescIterator;

	typedef std::map<string_t, TexDescIterator>    FileToTexDescMap;
	typedef std::map<DEV_TEXTURE, TexDescIterator> DevToTexDescMap;

	FileToTexDescMap _mapFile_to_TexDescIter;
	DevToTexDescMap  _mapDevTex_to_TexDescIter;
	TexDescList      _textures;
	std::map<string_t, size_t>   _mapName_to_Index;	// index in _LogicalTextures
	std::vector<LogicalTexture>  _LogicalTextures;

private:
	void Unload(TexDescIterator what);
	void CreateChecker(); // Create checker texture without name and with index=0

public:
	TextureManager();
	~TextureManager();

	bool LoadTexture(TexDescIterator &itTexDesc, const string_t &fileName);
    void UnloadAllTextures();

	int LoadPackage(const char* fileName);
	int LoadDirectory(const string_t &dirName, const string_t &texPrefix);

	size_t FindTexture(const char *name)   const;
	const LogicalTexture& get(size_t index) const;
	void bind(size_t index);

	void GetTextureNames(std::vector<string_t> &names, const char *prefix) const;
};

/////////////////////////////////////////////////////////////

class ThemeManager
{
	struct ThemeDesc
	{
		string_t fileName;
	};

	std::vector<ThemeDesc> _themes;

public:
	ThemeManager();
	~ThemeManager();

	size_t GetThemeCount();
	size_t FindTheme(const char *name);
	string_t GetThemeName(size_t index);

    bool ApplyTheme(size_t index);
};

typedef StaticSingleton<ThemeManager> _ThemeManager;


///////////////////////////////////////////////////////////////////////////////
// end of file
