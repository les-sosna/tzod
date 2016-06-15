#include "inc/video/TextureManager.h"
#include "inc/video/RenderBase.h"
#include "inc/video/ImageLoader.h"

#define TRACE(...)

#include <fs/FileSystem.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <cstring>

///////////////////////////////////////////////////////////////////////////////

class CheckerImage : public Image
{
public:
	// Image methods
	virtual const void* GetData() const override { return _bytes; }
	virtual unsigned int GetBpp() const override { return 24; }
	virtual unsigned int GetWidth() const override { return 4; }
	virtual unsigned int GetHeight() const override { return 4; }

private:
	static const unsigned char _bytes[];
};

const unsigned char CheckerImage::_bytes[] = {
	0,  0,  0,     0,  0,  0,   255,255,255,   255,255,255,
	0,  0,  0,     0,  0,  0,   255,255,255,   255,255,255,
	255,255,255,   255,255,255,     0,  0,  0,     0,  0,  0,
	255,255,255,   255,255,255,     0,  0,  0,     0,  0,  0,
};


///////////////////////////////////////////////////////////////////////////////

TextureManager::TextureManager(IRender &render)
    : _render(render)
{
	CreateChecker();
}

TextureManager::~TextureManager()
{
	UnloadAllTextures();
}

void TextureManager::UnloadAllTextures()
{
	for (auto &t: _devTextures)
		_render.TexFree(t.id);
	_devTextures.clear();
	_mapFile_to_TexDescIter.clear();
	_mapName_to_Index.clear();
	_logicalTextures.clear();
}

std::list<TextureManager::TexDesc>::iterator TextureManager::LoadTexture(const std::string &fileName, FS::FileSystem &fs)
{
	auto it = _mapFile_to_TexDescIter.find(fileName);
	if( _mapFile_to_TexDescIter.end() != it )
	{
		return it->second;
	}
	else
	{
		std::shared_ptr<FS::MemMap> file = fs.Open(fileName)->QueryMap();
		std::unique_ptr<Image> image(new TgaImage(file->GetData(), file->GetSize()));

		TexDesc td;
		if( !_render.TexCreate(td.id, *image) )
		{
			throw std::runtime_error("error in render device");
		}

		td.width     = image->GetWidth();
		td.height    = image->GetHeight();
		td.refCount  = 0;

		_devTextures.push_front(td);
		auto it2 = _devTextures.begin();
		_mapFile_to_TexDescIter.emplace(fileName, it2);
		return it2;
	}
}

void TextureManager::CreateChecker()
{
	TexDesc td;
	LogicalTexture tex;


	//
	// check if checker texture already exists
	//

	assert(_logicalTextures.empty()); // to be sure that checker will get index 0
	assert(_mapName_to_Index.empty());

	TRACE("Creating checker texture...");



	//
	// create device texture
	//

	CheckerImage c;
	if( !_render.TexCreate(td.id, c) )
	{
		TRACE("ERROR: error in render device");
		assert(false);
		return;
	}
	td.width     = c.GetWidth();
	td.height    = c.GetHeight();
	td.refCount  = 0;

	_devTextures.push_front(td);
	auto it = _devTextures.begin();



	//
	// create logical texture
	//

	tex.uvPivot = vec2d(0, 0);
	tex.pxFrameWidth = (float) td.width * 8;
	tex.pxFrameHeight = (float) td.height * 8;
	tex.pxBorderSize = 0;

	FRECT whole = {0,0,8,8};
	tex.uvFrames.push_back(whole);
	//---------------------
	_logicalTextures.emplace_back(tex, it);
	it->refCount++;
}

static int getint(lua_State *L, int tblidx, const char *field, int def)
{
	lua_getfield(L, tblidx, field);
	if( lua_isnumber(L, -1) )
		def = lua_tointeger(L, -1);
	lua_pop(L, 1); // pop result of getfield
	return def;
}

static float getfloat(lua_State *L, int tblidx, const char *field, float def)
{
	lua_getfield(L, tblidx, field);
	if( lua_isnumber(L, -1) )
		def = (float) lua_tonumber(L, -1);
	lua_pop(L, 1); // pop result of getfield
	return def;
}

static LogicalTexture getlt(lua_State *L, int idx, float pxWidth, float pxHeight)
{
	LogicalTexture tex;

	// texture bounds
	float uvLeft = floorf(getfloat(L, idx, "left", 0)) / pxWidth;
	float uvRight = floorf(getfloat(L, idx, "right", pxWidth)) / pxWidth;
	float uvTop = floorf(getfloat(L, idx, "top", 0)) / pxHeight;
	float uvBottom = floorf(getfloat(L, idx, "bottom", pxHeight)) / pxHeight;

	// border
	tex.pxBorderSize = floorf(getfloat(L, idx, "border", 0));
	float uvBorderWidth = tex.pxBorderSize / pxWidth;
	float uvBorderHeight = tex.pxBorderSize / pxHeight;

	// frames count
	int xframes = getint(L, idx, "xframes", 1);
	int yframes = getint(L, idx, "yframes", 1);

	// frame size with border
	float uvFrameWidth = (uvRight - uvLeft) / (float)xframes;
	float uvFrameHeight = (uvBottom - uvTop) / (float)yframes;

	// original size
	float scale_x = getfloat(L, idx, "xscale", 1);
	float scale_y = getfloat(L, idx, "yscale", 1);
	tex.pxFrameWidth = pxWidth * scale_x * uvFrameWidth;
	tex.pxFrameHeight = pxHeight * scale_y * uvFrameHeight;

	// pivot position
	tex.uvPivot.x = (float)getfloat(L, idx, "xpivot", pxWidth * uvFrameWidth / 2) / (pxWidth * uvFrameWidth);
	tex.uvPivot.y = (float)getfloat(L, idx, "ypivot", pxHeight * uvFrameHeight / 2) / (pxHeight * uvFrameHeight);

	// frames
	tex.uvFrames.reserve(xframes * yframes);
	for (int y = 0; y < yframes; ++y)
	{
		for (int x = 0; x < xframes; ++x)
		{
			FRECT rt;
			rt.left = uvLeft + uvFrameWidth * (float)x + uvBorderWidth;
			rt.right = uvLeft + uvFrameWidth * (float)(x + 1) - uvBorderWidth;
			rt.top = uvTop + uvFrameHeight * (float)y + uvBorderHeight;
			rt.bottom = uvTop + uvFrameHeight * (float)(y + 1) - uvBorderHeight;
			tex.uvFrames.push_back(rt);
		}
	}

	return tex;
}

int TextureManager::LoadPackage(const std::string &packageName, std::shared_ptr<FS::MemMap> file, FS::FileSystem &fs)
{
	TRACE("Loading texture package '%s'", packageName.c_str());

	lua_State *L = lua_open();

	if( 0 != (luaL_loadbuffer(L, file->GetData(), file->GetSize(), packageName.c_str()) || lua_pcall(L, 0, 1, 0)) )
	{
		TRACE("%s", lua_tostring(L, -1));
		lua_close(L);
		return 0;
	}

	if( !lua_istable(L, 1) )
	{
		lua_close(L);
		return 0;
	}

	// loop over files
	for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1

		// check that value is a table
		if( !lua_istable(L, -1) )
		{
			TRACE("WARNING: value is not a table; skipping.");
		}

		while( lua_istable(L, -1) )
		{
			lua_getfield(L, -1, "file");
			std::string fileName = lua_tostring(L, -1);
			lua_pop(L, 1); // pop result of lua_getfield

			std::list<TexDesc>::iterator texDescIter;
			try
			{
				texDescIter = LoadTexture(fileName, fs);
			}
			catch( const std::exception &e )
			{
				TRACE("WARNING: could not load texture '%s' - %s", f.c_str(), e.what());
				break;
			}

			lua_getfield(L, -1, "content");
			if( lua_istable(L, -1) )
			{
				// loop over textures in 'content' table
				for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
				{
					if( !lua_istable(L, -1) )
					{
						TRACE("WARNING: element of 'content' is not a table; skipping");
						continue;
					}

					// copy the key because lua_tostring may change its type
					lua_pushvalue(L, -2);
					if( const char *texname = lua_tostring(L, -1) )
					{
						// now 'value' at index -2
						LogicalTexture tex = getlt(L, -2, (float)texDescIter->width, (float)texDescIter->height);
						if( !tex.uvFrames.empty() )
						{
							texDescIter->refCount++;

							auto emplaced = _mapName_to_Index.emplace(texname, _logicalTextures.size());
							if( emplaced.second )
							{
								// define new texture
								_logicalTextures.emplace_back(tex, texDescIter);
							}
							else
							{
								// replace existing logical texture
								auto &existing = _logicalTextures[emplaced.first->second];
								assert(existing.second->refCount > 0);
								existing.first = tex;
								existing.second->refCount--;
								existing.second = texDescIter;
							}
						}
					}
					lua_pop(L, 1); // pop key copy
				} // loop over 'content'
			}
			else // if 'content' is table
			{
				TRACE("WARNING: 'content' field is not a table.");
			}
			lua_pop(L, 1); // pop the result of getfield("content")
			break;
		} // while( lua_istable(L, -1) )
	}
	lua_close(L);


	//
	// unload unused textures
	//

	for (auto it = _mapFile_to_TexDescIter.begin(); _mapFile_to_TexDescIter.end() != it; )
	{
		if (0 == it->second->refCount)
		{
			_devTextures.erase(it->second);
			it = _mapFile_to_TexDescIter.erase(it);
		}
		else
		{
			++it;
		}
	}

	TRACE("Total number of loaded textures: %d", _logicalTextures.size());
	return _logicalTextures.size();
}

int TextureManager::LoadDirectory(const std::string &dirName, const std::string &texPrefix, FS::FileSystem &fs)
{
	int count = 0;
	std::shared_ptr<FS::FileSystem> dir = fs.GetFileSystem(dirName);
	auto files = dir->EnumAllFiles("*.tga");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		std::string texName = texPrefix + *it;
		texName.erase(texName.length() - 4); // cut out the file extension

		if (_mapName_to_Index.end() != _mapName_to_Index.find(texName))
			continue; // skip if there is a texture with the same name

		std::list<TexDesc>::iterator texDescIter;
		std::string fileName = dirName + '/' + *it;
		try
		{
			texDescIter = LoadTexture(fileName, fs);
		}
		catch( const std::exception &e )
		{
			TRACE("WARNING: could not load texture '%s' - %s", fileName.c_str(), e.what());
			continue;
		}

		LogicalTexture tex;
		tex.uvPivot  = vec2d(0.5f, 0.5f);
		tex.pxFrameWidth = (float) texDescIter->width;
		tex.pxFrameHeight = (float) texDescIter->height;
		tex.pxBorderSize = 0;
		tex.uvFrames = { { 0, 0, 1, 1 } };

		_mapName_to_Index[texName] = _logicalTextures.size();
		_logicalTextures.emplace_back(tex, texDescIter);
		texDescIter->refCount++;
		count++;
	}
	return count;
}

size_t TextureManager::FindSprite(const std::string &name) const
{
	std::map<std::string, size_t>::const_iterator it = _mapName_to_Index.find(name);
	if( _mapName_to_Index.end() != it )
		return it->second;

	// flood the console
	TRACE("texture '%s' not found!", name.c_str());

	return 0; // index of checker texture
}

void TextureManager::GetTextureNames(std::vector<std::string> &names,
                                     const char *prefix) const
{
	size_t trimLength = prefix ? std::strlen(prefix) : 0;

	names.clear();
	std::map<std::string, size_t>::const_iterator it = _mapName_to_Index.begin();
	for(; it != _mapName_to_Index.end(); ++it )
	{
		if( prefix && 0 != it->first.find(prefix) )
			continue;
		names.push_back(it->first.substr(trimLength));
	}
}

float TextureManager::GetCharHeight(size_t fontTexture) const
{
	return GetSpriteInfo(fontTexture).pxFrameHeight;
}

