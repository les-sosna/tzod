// TextureManager.h

#pragma once

#include "RenderBase.h"

#include <list>
#include <map>
#include <memory>
#include <vector>
#include <string>

namespace FS
{
	class MemMap;
	class FileSystem;
}

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
};

class TextureManager
{
public:
	TextureManager(IRender &render);
	~TextureManager();

	IRender& GetRender() const { return _render; }

	int LoadPackage(const std::string &packageName, std::shared_ptr<FS::MemMap> file, FS::FileSystem &fs);
	int LoadDirectory(const std::string &dirName, const std::string &texPrefix, FS::FileSystem &fs);
	void UnloadAllTextures();

	size_t FindSprite(const std::string &name) const;
	const LogicalTexture& GetSpriteInfo(size_t texIndex) const { return _logicalTextures[texIndex]; }
	float GetFrameWidth(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].pxFrameWidth; }
	float GetFrameHeight(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].pxFrameHeight; }
	unsigned int GetFrameCount(size_t texIndex) const { return _logicalTextures[texIndex].xframes * _logicalTextures[texIndex].yframes; }

	void GetTextureNames(std::vector<std::string> &names, const char *prefix, bool noPrefixReturn) const;

	float GetCharHeight(size_t fontTexture) const;

protected:
    IRender &_render;

	struct TexDesc
	{
		DEV_TEXTURE id;
		int width;          // The Width Of The Entire Image.
		int height;         // The Height Of The Entire Image.
		int refCount;       // number of logical textures
	};
	typedef std::list<TexDesc>       TexDescList;
	typedef TexDescList::iterator    TexDescIterator;

	typedef std::map<std::string, TexDescIterator>    FileToTexDescMap;
	typedef std::map<DEV_TEXTURE, TexDescIterator> DevToTexDescMap;

	FileToTexDescMap _mapFile_to_TexDescIter;
	DevToTexDescMap  _mapDevTex_to_TexDescIter;
	TexDescList      _textures;
	std::map<std::string, size_t>   _mapName_to_Index;// index in _logicalTextures
	std::vector<LogicalTexture>  _logicalTextures;

	void LoadTexture(TexDescIterator &itTexDesc, const std::string &fileName, FS::FileSystem &fs);
	void Unload(TexDescIterator what);

	void CreateChecker(); // Create checker texture without name and with index=0
};

// end of file
