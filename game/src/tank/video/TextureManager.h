// TextureManager.h

#pragma once

#include "RenderBase.h"

struct LogicalTexture
{
	DEV_TEXTURE dev_texture;

	float uvLeft;
	float uvTop;
	float uvRight;
	float uvBottom;
	float uvFrameWidth;
	float uvFrameHeight;
	vec2d uvPivot;

	float pxFrameWidth;
	float pxFrameHeight;
	int   xframes;
	int   yframes;

	std::vector<FRECT> uvFrames;

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
	std::map<string_t, size_t>   _mapName_to_Index;// index in _logicalTextures
	std::vector<LogicalTexture>  _logicalTextures;

private:
	void Unload(TexDescIterator what);
	void CreateChecker(); // Create checker texture without name and with index=0

public:
	TextureManager();
	~TextureManager();

	void LoadTexture(TexDescIterator &itTexDesc, const string_t &fileName);
    void UnloadAllTextures();

	int LoadPackage(const string_t &fileName);
	int LoadDirectory(const string_t &dirName, const string_t &texPrefix);

	size_t FindTexture(const char *name)   const;
	const LogicalTexture& Get(size_t index) const { return _logicalTextures[index]; }

	bool IsValidTexture(size_t index) const;

	void GetTextureNames(std::vector<string_t> &names, const char *prefix, bool noPrefixReturn) const;

	float GetCharHeight(size_t fontTexture) const;

	void DrawBitmapText(size_t tex, const string_t &str, SpriteColor color, float x, float y, enumAlignText align = alignTextLT) const;
	void DrawSprite(size_t tex, unsigned int frame, SpriteColor color, float x, float y, float rot) const;
	void DrawIndicator(size_t tex, float x, float y, float value) const;
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
	size_t FindTheme(const string_t &name);
	string_t GetThemeName(size_t index);

    bool ApplyTheme(size_t index);
};

typedef StaticSingleton<ThemeManager> _ThemeManager;


///////////////////////////////////////////////////////////////////////////////
// end of file
