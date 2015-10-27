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
	virtual const void* GetData() const { return _bytes; }
	virtual unsigned long GetBpp() const { return 24; }
	virtual unsigned long GetWidth() const { return 4; }
	virtual unsigned long GetHeight() const { return 4; }

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
	TexDescIterator it = _textures.begin();
	while( it != _textures.end() )
		_render.TexFree((it++)->id);
	_textures.clear();
	_mapFile_to_TexDescIter.clear();
	_mapDevTex_to_TexDescIter.clear();
	_mapName_to_Index.clear();
	_logicalTextures.clear();
}

void TextureManager::LoadTexture(TexDescIterator &itTexDesc, const std::string &fileName, FS::FileSystem &fs)
{
	FileToTexDescMap::iterator it = _mapFile_to_TexDescIter.find(fileName);
	if( _mapFile_to_TexDescIter.end() != it )
	{
		itTexDesc = it->second;
	}
	else
	{
		std::shared_ptr<FS::MemMap> file = fs.Open(fileName)->QueryMap();
		std::unique_ptr<TgaImage> image(new TgaImage(file->GetData(), file->GetSize()));

		TexDesc td;
		if( !_render.TexCreate(td.id, *image) )
		{
			throw std::runtime_error("error in render device");
		}

		td.width     = image->GetWidth();
		td.height    = image->GetHeight();
		td.refCount  = 0;

		_textures.push_front(td);
		itTexDesc = _textures.begin();
		_mapFile_to_TexDescIter[fileName] = itTexDesc;
		_mapDevTex_to_TexDescIter[itTexDesc->id] = itTexDesc;
	}
}

void TextureManager::Unload(TexDescIterator what)
{
	_render.TexFree(what->id);

	FileToTexDescMap::iterator it = _mapFile_to_TexDescIter.begin();
	while( _mapFile_to_TexDescIter.end() != it )
	{
		if( it->second->id == what->id )
		{
			_mapFile_to_TexDescIter.erase(it);
			break;
		}
		++it;
	}

	_mapDevTex_to_TexDescIter.erase(what->id);
	_textures.erase(what);
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

	_textures.push_front(td);
	TexDescIterator it = _textures.begin();



	//
	// create logical texture
	//

	tex.dev_texture = it->id;
	tex.uvLeft   = 0;
	tex.uvTop    = 0;
	tex.uvRight  = 1;
	tex.uvBottom = 1;
	tex.xframes  = 1;
	tex.yframes  = 1;
	tex.uvFrameWidth  = 8.0f;
	tex.uvFrameHeight = 8.0f;
	tex.uvPivot = vec2d(0, 0);
	tex.pxFrameWidth  = (float) td.width * tex.uvFrameWidth;
	tex.pxFrameHeight = (float) td.height * tex.uvFrameHeight;

	FRECT whole = {0,0,1,1};
	tex.uvFrames.push_back(whole);
	//---------------------
	_logicalTextures.push_back(tex);
	it->refCount++;
}

static int auxgetint(lua_State *L, int tblidx, const char *field, int def)
{
	lua_getfield(L, tblidx, field);
	if( lua_isnumber(L, -1) ) def = lua_tointeger(L, -1);
	lua_pop(L, 1); // pop result of getfield
	return def;
}

static float auxgetfloat(lua_State *L, int tblidx, const char *field, float def)
{
	lua_getfield(L, tblidx, field);
	if( lua_isnumber(L, -1) ) def = (float) lua_tonumber(L, -1);
	lua_pop(L, 1); // pop result of getfield
	return def;
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
			TexDescIterator td;

			// get a file name; load
			lua_getfield(L, -1, "file");
			std::string f = lua_tostring(L, -1);
			lua_pop(L, 1); // pop result of lua_getfield

			try
			{
				LoadTexture(td, f, fs);
			}
			catch( const std::exception &e )
			{
				TRACE("WARNING: could not load texture '%s' - %s", f.c_str(), e.what());
				break;
			}


			// get 'content' field
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

					lua_pushvalue(L, -2); // create copy of the key
					if( const char *texname = lua_tostring(L, -1) )
					{
						// now 'value' at index -2

						float scale_x = auxgetfloat(L, -2, "xscale", 1);
						float scale_y = auxgetfloat(L, -2, "yscale", 1);

						LogicalTexture tex;
						tex.dev_texture = td->id;

						// texture bounds
						tex.uvLeft   = (float) floorf(auxgetfloat(L, -2, "left", 0)) / (float) td->width;
						tex.uvRight  = (float) floorf(auxgetfloat(L, -2, "right", (float) td->width)) / (float) td->width;
						tex.uvTop    = (float) floorf(auxgetfloat(L, -2, "top", 0)) / (float) td->height;
						tex.uvBottom = (float) floorf(auxgetfloat(L, -2, "bottom", (float) td->height)) / (float) td->height;

						// frames count
						tex.xframes = auxgetint(L, -2, "xframes", 1);
						tex.yframes = auxgetint(L, -2, "yframes", 1);

						// frame size
						tex.uvFrameWidth  = (tex.uvRight - tex.uvLeft) / (float) tex.xframes;
						tex.uvFrameHeight = (tex.uvBottom - tex.uvTop) / (float) tex.yframes;

						// original size
						tex.pxFrameWidth  = (float) td->width  * scale_x * tex.uvFrameWidth;
						tex.pxFrameHeight = (float) td->height * scale_y * tex.uvFrameHeight;

						// pivot position
						tex.uvPivot.x = (float) auxgetfloat(L, -2, "xpivot", (float) td->width * tex.uvFrameWidth / 2) / ((float) td->width * tex.uvFrameWidth);
						tex.uvPivot.y = (float) auxgetfloat(L, -2, "ypivot", (float) td->height * tex.uvFrameHeight / 2) / ((float) td->height * tex.uvFrameHeight);

						// frames
						tex.uvFrames.reserve(tex.xframes * tex.yframes);
						for( int y = 0; y < tex.yframes; ++y )
						{
							for( int x = 0; x < tex.xframes; ++x )
							{
								FRECT rt;
								rt.left   = tex.uvLeft + tex.uvFrameWidth * (float) x;
								rt.right  = tex.uvLeft + tex.uvFrameWidth * (float) (x + 1);
								rt.top    = tex.uvTop + tex.uvFrameHeight * (float) y;
								rt.bottom = tex.uvTop + tex.uvFrameHeight * (float) (y + 1);
								tex.uvFrames.push_back(rt);
							}
						}

						//---------------------
						if( tex.xframes > 0 && tex.yframes > 0 )
						{
							td->refCount++;
							//---------------------------------------------
							std::map<std::string, size_t>::iterator it =
								_mapName_to_Index.find(texname);

							if( _mapName_to_Index.end() != it )
							{
								// replace existing logical texture
								LogicalTexture &existing = _logicalTextures[it->second];
								TexDescIterator tmp =
									_mapDevTex_to_TexDescIter[existing.dev_texture];
								existing = tex;
								tmp->refCount--;
								assert(tmp->refCount >= 0);
							}
							else
							{
								// define new texture
								_mapName_to_Index[texname] = _logicalTextures.size();
								_logicalTextures.push_back(tex);
							}
						} // end if( xframes > 0 && yframes > 0 )
					} // end if( texname )
					lua_pop(L, 1); // remove copy of the key
				} // end loop over 'content'
			} // end if 'content' is table
			else
			{
				TRACE("WARNING: 'content' field is not a table.");
			}
			lua_pop(L, 1); // pop the result of getfield("content")
			break;
		} // end of while( lua_istable(L, -1) )
	}
	lua_close(L);


	//
	// unload unused textures
	//
	TexDescIterator it = _textures.begin();
	while( _textures.end() != it )
	{
        TexDescIterator tmp = it++;
		assert(tmp->refCount >= 0);
		if( 0 == tmp->refCount )
			Unload(tmp);
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
		TexDescIterator td;
		std::string fileName = dirName + '/' + *it;
		try
		{
			LoadTexture(td, fileName, fs);
		}
		catch( const std::exception &e )
		{
			TRACE("WARNING: could not load texture '%s' - %s", fileName.c_str(), e.what());
			continue;
		}

		std::string texName = texPrefix + *it;
		texName.erase(texName.length() - 4); // cut out the file extension

		LogicalTexture tex;
		tex.dev_texture = td->id;
		tex.uvLeft   = 0;
		tex.uvTop    = 0;
		tex.uvRight  = 1;
		tex.uvBottom = 1;
		tex.uvPivot  = vec2d(0.5f, 0.5f);
		tex.xframes  = 1;
		tex.yframes  = 1;
		tex.uvFrameWidth  = 1;
		tex.uvFrameHeight = 1;
		tex.pxFrameWidth  = (float) td->width;
		tex.pxFrameHeight = (float) td->height;

		FRECT frame = {0,0,1,1};
		tex.uvFrames.push_back(frame);
		//---------------------
		if( _mapName_to_Index.end() != _mapName_to_Index.find(texName) )
			continue; // skip if there is a texture with the same name
		_mapName_to_Index[texName] = _logicalTextures.size();
		_logicalTextures.push_back(tex);
		td->refCount++;
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
                                     const char *prefix, bool noPrefixReturn) const
{
	size_t trimLength = (prefix && noPrefixReturn) ? std::strlen(prefix) : 0;

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

// end of file
