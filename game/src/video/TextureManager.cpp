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
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////

class CheckerImage final
	: public Image
{
public:
	// Image
	const void* GetData() const override { return _bytes; }
	unsigned int GetBpp() const override { return 24; }
	unsigned int GetWidth() const override { return 4; }
	unsigned int GetHeight() const override { return 4; }

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
{
	CreateChecker(render);
}

TextureManager::~TextureManager()
{
	assert(_devTextures.empty());
}

void TextureManager::UnloadAllTextures(IRender& render) noexcept
{
	for (auto &t: _devTextures)
		render.TexFree(t.id);
	_devTextures.clear();
	_mapPath_to_TexDescIter.clear();
	_mapName_to_Index.clear();
	_logicalTextures.clear();
}

std::list<TextureManager::TexDesc>::iterator TextureManager::LoadTexture(IRender& render, FS::FileSystem& fs, std::string_view fileName, bool magFilter)
{
	auto it = _mapPath_to_TexDescIter.find(fileName);
	if( _mapPath_to_TexDescIter.end() != it )
	{
		return it->second;
	}
	else
	{
		auto file = fs.Open(fileName)->QueryMap();
		TgaImage image(file->GetData(), file->GetSize());

		TexDesc td;
		if( !render.TexCreate(td.id, image, magFilter) )
		{
			throw std::runtime_error("error in render device");
		}

		td.width = image.GetWidth();
		td.height = image.GetHeight();
		td.refCount = 0;

		_devTextures.push_front(td);
		auto it2 = _devTextures.begin();
		_mapPath_to_TexDescIter.emplace(fileName, it2);
		return it2;
	}
}

void TextureManager::CreateChecker(IRender& render)
{
	assert(_logicalTextures.empty()); // to be sure that checker will get index 0
	assert(_mapName_to_Index.empty());
	TRACE("Creating checker texture...");

	TexDesc td;
	CheckerImage c;
	if( !render.TexCreate(td.id, c, false) )
	{
		TRACE("ERROR: error in render device");
		assert(false);
		return;
	}
	td.width = c.GetWidth();
	td.height = c.GetHeight();
	td.refCount = 0;

	_devTextures.push_front(td);

	auto texDescIter = _devTextures.begin();
	texDescIter->refCount++;

	LogicalTexture tex;
	tex.pxPivot = vec2d{ (float)td.width, (float)td.height } * 4;
	tex.pxFrameWidth = (float) td.width * 8;
	tex.pxFrameHeight = (float) td.height * 8;
	tex.pxBorderSize = 0;
	tex.uvFrames = { { 0,0,2,2 } };

	_logicalTextures.emplace_back(tex, texDescIter);
}

static int getint(lua_State *L, int tblidx, const char *field, int def)
{
	lua_getfield(L, tblidx, field);
	if( lua_isnumber(L, -1) )
		def = lua_tointeger(L, -1);
	lua_pop(L, 1); // pop result of getfield
	return def;
}

static bool trygetfloat(lua_State* L, int tblidx, const char* field, float def, float *outValue)
{
	bool result;
	lua_getfield(L, tblidx, field);
	if (lua_isnumber(L, -1))
	{
		*outValue = (float)lua_tonumber(L, -1);
		result = true;
	}
	else
	{
		*outValue = def;
		result = false;
	}
	lua_pop(L, 1); // pop result of getfield
	return result;
}

static bool getbool(lua_State *L, int tblidx, const char *field, bool def)
{
	lua_getfield(L, tblidx, field);
	if( !lua_isnil(L, -1) )
		def = !!lua_toboolean(L, -1);
	lua_pop(L, 1); // pop result of getfield
	return def;
}

static void getspritedef(lua_State *L, int idx, SpriteDefinition *outSpriteDef)
{
	// texture bounds
	trygetfloat(L, idx, "left", 0, &outSpriteDef->atlasOffset.x);
	trygetfloat(L, idx, "top", 0, &outSpriteDef->atlasOffset.y);
	outSpriteDef->hasSizeX = trygetfloat(L, idx, "right", 0, &outSpriteDef->atlasSize.x);
	outSpriteDef->hasSizeY = trygetfloat(L, idx, "bottom", 0, &outSpriteDef->atlasSize.y);
	outSpriteDef->atlasSize -= outSpriteDef->atlasOffset;

	// border
	trygetfloat(L, idx, "border", 0, &outSpriteDef->border);

	// scale
	trygetfloat(L, idx, "xscale", 1, &outSpriteDef->scale.x);
	trygetfloat(L, idx, "yscale", 1, &outSpriteDef->scale.y);

	// frames count
	outSpriteDef->xframes = getint(L, idx, "xframes", 1);
	outSpriteDef->yframes = getint(L, idx, "yframes", 1);

	// pivot position
	outSpriteDef->hasPivotX = trygetfloat(L, idx, "xpivot", 0, &outSpriteDef->pivot.x);
	outSpriteDef->hasPivotY = trygetfloat(L, idx, "ypivot", 0, &outSpriteDef->pivot.y);

	// filter
	outSpriteDef->magFilter = getbool(L, idx, "magfilter", false);
}

static LogicalTexture LogicalTextureFromSpriteDefinition(const SpriteDefinition& sd, vec2d pxTextureSize)
{
	LogicalTexture lt;

	// sprite size with border
	vec2d pxAtlasSize = { sd.hasSizeX ? sd.atlasSize.x : pxTextureSize.x - sd.atlasOffset.x, sd.hasSizeY ? sd.atlasSize.y : pxTextureSize.y - sd.atlasOffset.y };
	vec2d pxFrameSize = pxAtlasSize / vec2d{ (float)sd.xframes, (float)sd.yframes };
	vec2d uvFrameSize = pxFrameSize / pxTextureSize;

	lt.pxPivot = vec2d{ sd.hasPivotX ? sd.pivot.x : pxFrameSize.x / 2, sd.hasPivotY ? sd.pivot.y : pxFrameSize.y / 2 } * sd.scale;

	// original size
	lt.pxFrameWidth = pxTextureSize.x * sd.scale.x * uvFrameSize.x;
	lt.pxFrameHeight = pxTextureSize.y * sd.scale.y * uvFrameSize.y;
	lt.pxBorderSize = sd.border;

	// frames
	vec2d uvBorderSize = { sd.border / pxTextureSize.x, sd.border / pxTextureSize.y };
	vec2d uvOffset = sd.atlasOffset / pxTextureSize;
	lt.uvFrames.reserve(sd.xframes * sd.yframes);
	for (int y = 0; y < sd.yframes; ++y)
	{
		for (int x = 0; x < sd.xframes; ++x)
		{
			FRECT rt;
			rt.left = uvOffset.x + uvFrameSize.x * (float)x + uvBorderSize.x;
			rt.top = uvOffset.y + uvFrameSize.y * (float)y + uvBorderSize.y;
			rt.right = uvOffset.x + uvFrameSize.x * (float)(x + 1) - uvBorderSize.x;
			rt.bottom = uvOffset.y + uvFrameSize.y * (float)(y + 1) - uvBorderSize.y;
			lt.uvFrames.push_back(rt);
		}
	}

	return lt;
}

#include <luaetc/LuaDeleter.h>

std::vector<SpriteDefinition> ParsePackage(const std::string &packageName, std::shared_ptr<FS::MemMap> file, FS::FileSystem &fs)
{
	std::vector<SpriteDefinition> result;

	std::unique_ptr<lua_State, LuaStateDeleter> luaState(lua_open());
	if (!luaState)
		throw std::bad_alloc();

	lua_State *L = luaState.get();

	if (0 != (luaL_loadbuffer(L, static_cast<const char *>(file->GetData()), file->GetSize(), packageName.c_str()) ||
	    lua_pcall(L, 0, 1, 0)))
	{
		std::runtime_error e(lua_tostring(L, -1));
		lua_close(L);
		throw e;
	}

	if (lua_istable(L, -1))
	{
		// loop over files
		for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
		{
			// now 'key' is at index -2 and 'value' at index -1
			if (!lua_istable(L, -1))
				continue;

			lua_getfield(L, -1, "file");
			std::string fileName = lua_tostring(L, -1);
			lua_pop(L, 1); // pop result of lua_getfield

			lua_getfield(L, -1, "content");
			if (lua_istable(L, -1))
			{
				// loop over textures in 'content' table
				for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
				{
					if (!lua_istable(L, -1))
						continue;

					// make the key copy because lua_tostring may change its type
					lua_pushvalue(L, -2);
					if (const char *texname = lua_tostring(L, -1))
					{
						// now 'value' at index -2
						SpriteDefinition sd = {};
						getspritedef(L, -2, &sd);
						sd.textureFilePath = fileName;
						sd.spriteName = texname;

						result.push_back(std::move(sd));
					}
					lua_pop(L, 1); // pop key copy
				}
			}
			lua_pop(L, 1); // pop content
		}
	}

	return result;
}

int TextureManager::LoadPackage(IRender& render, FS::FileSystem& fs, const std::vector<SpriteDefinition> &definitions)
{
	for (auto &item: definitions)
	{
		std::list<TexDesc>::iterator texDescIt = LoadTexture(render, fs, item.textureFilePath, item.magFilter);
		texDescIt->refCount++;

		LogicalTexture lt = LogicalTextureFromSpriteDefinition(item, vec2d{ (float)texDescIt->width, (float)texDescIt->height });

		auto emplaced = _mapName_to_Index.emplace(item.spriteName, _logicalTextures.size());
		if( emplaced.second )
		{
			// define new texture
			_logicalTextures.emplace_back(lt, texDescIt);
		}
		else
		{
			// replace existing logical texture
			auto &existing = _logicalTextures[emplaced.first->second];
			assert(existing.second->refCount > 0);
			existing.first = lt;
			existing.second->refCount--;
			existing.second = texDescIt;
		}
	}

	// unload unused textures
	for (auto it = _mapPath_to_TexDescIter.begin(); _mapPath_to_TexDescIter.end() != it; )
	{
		if (0 == it->second->refCount)
		{
			render.TexFree(it->second->id);
			_devTextures.erase(it->second);
			it = _mapPath_to_TexDescIter.erase(it);
		}
		else
		{
			++it;
		}
	}

	TRACE("Total number of loaded textures: %d", _logicalTextures.size());
	return _logicalTextures.size();
}

std::vector<SpriteDefinition> ParseDirectory(const std::string &dirName, const std::string &texPrefix, FS::FileSystem &fs)
{
	std::vector<SpriteDefinition> result;

	std::shared_ptr<FS::FileSystem> dir = fs.GetFileSystem(dirName);
	auto files = dir->EnumAllFiles("*.tga");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		std::string texName = texPrefix + *it;
		texName.erase(texName.length() - 4); // cut out the file extension

		SpriteDefinition sd = {};
		sd.textureFilePath = dirName + '/' + *it;
		sd.spriteName = std::move(texName);
		sd.scale = vec2d{ 1, 1 };
		sd.xframes = 1;
		sd.yframes = 1;
		sd.magFilter = true;

		result.push_back(std::move(sd));
	}

	return result;
}

size_t TextureManager::FindSprite(std::string_view name) const
{
	std::map<std::string, size_t>::const_iterator it = _mapName_to_Index.find(name);
	if( _mapName_to_Index.end() != it )
		return it->second;

//	TRACE("texture '%s' not found!", name.c_str());

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

float TextureManager::GetCharWidth(size_t fontTexture) const
{
	return GetSpriteInfo(fontTexture).pxFrameWidth - 1;
}
