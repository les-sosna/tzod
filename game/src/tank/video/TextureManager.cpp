// TextureManager.cpp

#include "stdafx.h"

#include "TextureManager.h"
#include "RenderBase.h"
#include "ImageLoader.h"

#include "macros.h"
#include "level.h"  // FIXME

#include "core/debug.h"
#include "core/Application.h"
#include "core/Console.h"

#include "gc/2dSprite.h"

#include "fs/FileSystem.h"

///////////////////////////////////////////////////////////////////////////////

class CheckerImage : public Image
{
public:
	CheckerImage() {}

	// Image
	virtual const void* GetData() const { return _bytes; }
	virtual long GetBpp() const { return 24; }
	virtual long GetWidth() const { return 4; }
	virtual long GetHeight() const { return 4; }

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

TextureManager::TextureManager()
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
		g_render->TexFree((it++)->id);
	_textures.clear();
	_mapFile_to_TexDescIter.clear();
	_mapDevTex_to_TexDescIter.clear();
	_mapName_to_Index.clear();
	_LogicalTextures.clear();
}

bool TextureManager::LoadTexture(TexDescIterator &itTexDesc, const string_t &fileName)
{
	FileToTexDescMap::iterator it = _mapFile_to_TexDescIter.find(fileName);
	if( _mapFile_to_TexDescIter.end() != it )
	{
		itTexDesc = it->second;
		return true;
	}

	SafePtr<FS::File> file = g_fs->Open(fileName);
	SafePtr<TgaImage> image(new TgaImage(file->GetData(), file->GetSize()));

	TexDesc td;
	if( !g_render->TexCreate(td.id, GetRawPtr(image)) )
	{
		TRACE("ERROR: '%s' - error in render device\n", fileName.c_str());
		return false;
	}

	td.width     = image->GetWidth();
	td.height    = image->GetHeight();
	td.refCount  = 0;

	_textures.push_front(td);
	itTexDesc = _textures.begin();
	_mapFile_to_TexDescIter[fileName] = itTexDesc;
	_mapDevTex_to_TexDescIter[itTexDesc->id] = itTexDesc;

	return true;
}

void TextureManager::Unload(TexDescIterator what)
{
	g_render->TexFree(what->id);

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

	static BYTE bytes[] = {
	    0,  0,  0,     0,  0,  0,   255,255,255,   255,255,255,
	    0,  0,  0,     0,  0,  0,   255,255,255,   255,255,255,
	  255,255,255,   255,255,255,     0,  0,  0,     0,  0,  0,
	  255,255,255,   255,255,255,     0,  0,  0,     0,  0,  0,
	};



	//
	// check if checker texture already exists
	//

	_ASSERT(_LogicalTextures.empty()); // to be sure that checker will get index 0
	_ASSERT(_mapName_to_Index.empty());

	TRACE("Creating checker texture\n");



	//
	// create device texture
	//

	SafePtr<CheckerImage> c(new CheckerImage());

	if( !g_render->TexCreate(td.id, GetRawPtr(c) ) )
	{
		TRACE("ERROR: error in render device\n");
		_ASSERT(FALSE);
		return;
	}

	td.width     = c->GetWidth();
	td.height    = c->GetHeight();
	td.refCount  = 0;

	_textures.push_front(td);
	TexDescIterator it = _textures.begin();



	//
	// create logical texture
	//

	tex.dev_texture = it->id;
	tex.left   = 0;
	tex.top    = 0;
	tex.right  = (float) td.width;
	tex.bottom = (float) td.height;
	tex.xframes  = 1;
	tex.yframes  = 1;
	tex.frame_width  = 8.0f * (float) td.width;
	tex.frame_height = 8.0f * (float) td.height;
	tex.pixel_width  = 1.0f / tex.frame_width;
	tex.pixel_height = 1.0f / tex.frame_height;
	tex.color = 0xffffffff;
	//---------------------
    _LogicalTextures.push_back(tex);
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

int TextureManager::LoadPackage(const string_t &filename)
{
	TRACE("Loading texture package '%s'\n", filename.c_str());

	lua_State *L = lua_open();

	if( 0 != (luaL_loadfile(L, filename.c_str()) || lua_pcall(L, 0, 1, 0)) )
	{
		TRACE("%s\n", lua_tostring(L, -1));
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
			TRACE("WARNING: value is not a table; skipping.\n");
		}

		while( lua_istable(L, -1) )
		{
			TexDescIterator td;

			// get a file name; load
			lua_getfield(L, -1, "file");
			bool result = LoadTexture(td, lua_tostring(L, -1));
			lua_pop(L, 1); // pop result of lua_getfield

			if( !result )
			{
				// couldn't load TGA
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
						TRACE("WARNING: element of 'content' is not a table; skipping\n");
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
						tex.left   = (float) floorf(auxgetfloat(L, -2, "left", 0)) * scale_x;
						tex.right  = (float) floorf(auxgetfloat(L, -2, "right", (float) td->width)) * scale_x;
						tex.top    = (float) floorf(auxgetfloat(L, -2, "top", 0)) * scale_y;
						tex.bottom = (float) floorf(auxgetfloat(L, -2, "bottom", (float) td->height)) * scale_y;

						// frames count
						tex.xframes = auxgetint(L, -2, "xframes", 1);
						tex.yframes = auxgetint(L, -2, "yframes", 1);

						// frame size
						tex.frame_width  = (tex.right - tex.left) / (float) tex.xframes;
						tex.frame_height = (tex.bottom - tex.top) / (float) tex.yframes;

						// pivot position
						tex.pivot_x = (float) auxgetfloat(L, -2, "xpivot", tex.frame_width/2/scale_y) * scale_x;
						tex.pivot_y = (float) auxgetfloat(L, -2, "ypivot", tex.frame_height/2/scale_y) * scale_y;

						// pixel size
						tex.pixel_width  = 1.0f / (float) td->width  / scale_x;
						tex.pixel_height = 1.0f / (float) td->height / scale_y;

						// sprite color
						tex.color.dwColor = (DWORD) auxgetint(L, -2, "color", 0xffffffff);

						//---------------------
						if( tex.xframes > 0 && tex.yframes > 0 )
						{
							td->refCount++;
							//---------------------------------------------
							std::map<string_t, size_t>::iterator it =
								_mapName_to_Index.find(texname);

							if( _mapName_to_Index.end() != it )
							{
								// replace existing logical texture
								LogicalTexture &existing = _LogicalTextures[it->second];
								TexDescIterator tmp =
									_mapDevTex_to_TexDescIter[existing.dev_texture];
								existing = tex;
								tmp->refCount--;
								_ASSERT(tmp->refCount >= 0);
							}
							else
							{
								// define new texture
								_mapName_to_Index[texname] = _LogicalTextures.size();
								_LogicalTextures.push_back(tex);
							}
						} // end if( xframes > 0 && yframes > 0 )
					} // end if( texname )
					lua_pop(L, 1); // remove copy of the key
				} // end loop over 'content'
			} // end if 'content' is table
			else
			{
				TRACE("WARNING: 'content' field is not a table.\n");
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
		_ASSERT(tmp->refCount >= 0);
		if( 0 == tmp->refCount )
			Unload(tmp);
	}

	TRACE("Total number of loaded textures: %d\n", _LogicalTextures.size());
	return _LogicalTextures.size();
}

int TextureManager::LoadDirectory(const string_t &dirName, const string_t &texPrefix)
{
	int count = 0;

	SafePtr<FS::FileSystem> dir = g_fs->GetFileSystem(dirName);

	std::set<string_t> files;
	dir->EnumAllFiles(files, TEXT("*.tga"));

	for( std::set<string_t>::iterator it = files.begin(); it != files.end(); ++it )
	{
		TexDescIterator td;
		if( LoadTexture(td, dirName + TEXT("/") + *it) )
		{
			string_t texName = texPrefix + *it;
			texName.erase(texName.length() - 4); // cut out the file extension

			LogicalTexture tex;
			tex.dev_texture = td->id;
			tex.left   = 0;
			tex.top    = 0;
			tex.right  = (float) td->width;
			tex.bottom = (float) td->height;
			tex.xframes  = 1;
			tex.yframes  = 1;
			tex.frame_width  = (float) td->width;
			tex.frame_height = (float) td->height;
			tex.pixel_width  = 1.0f / (float) td->width;
			tex.pixel_height = 1.0f / (float) td->height;
			tex.color = 0xffffffff;
			//---------------------
			if( _mapName_to_Index.end() != _mapName_to_Index.find(texName) )
				continue; // текстура с таким именем уже есть. пропускаем.
			_mapName_to_Index[texName] = _LogicalTextures.size();
            _LogicalTextures.push_back(tex);
			td->refCount++;
			count++;
		}
	}
	return count;
}

size_t TextureManager::FindTexture(const char *name) const
{
	std::map<string_t, size_t>::const_iterator it = _mapName_to_Index.find(name);
	if( _mapName_to_Index.end() != it )
		return it->second;

	// flood the console
	g_app->GetConsole()->printf("texture '%s' not found!\n", name);

	return 0; // index of checker texture
}

const LogicalTexture& TextureManager::Get(size_t index) const
{
	_ASSERT(index < _LogicalTextures.size());
	return _LogicalTextures[index];
}

void TextureManager::Bind(size_t index)
{
	_ASSERT(index < _LogicalTextures.size());
	g_render->TexBind(_LogicalTextures[index].dev_texture);
}

bool TextureManager::IsValidTexture(size_t index) const
{
	return index < _LogicalTextures.size();
}

void TextureManager::GetTextureNames(std::vector<string_t> &names,
									 const char *prefix, bool noPrefixReturn) const
{
	size_t trimLength = (prefix && noPrefixReturn) ? strlen(prefix) : 0;

	names.clear();
	std::map<string_t, size_t>::const_iterator it = _mapName_to_Index.begin();
	for(; it != _mapName_to_Index.end(); ++it )
	{
		if( prefix && 0 != it->first.find(prefix) )
			continue;
		names.push_back(it->first.substr(trimLength));
	}
}

////////////////////////////////////////////////////////////////////

ThemeManager::ThemeManager()
{
	TCHAR curdir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, curdir);

	if( !SetCurrentDirectory(DIR_THEMES) )
		return;

	WIN32_FIND_DATA fd = {0};
	HANDLE hSearch = FindFirstFile("*.lua", &fd);
	if( INVALID_HANDLE_VALUE != hSearch )
	{
		do {
			ThemeDesc td;
			td.fileName = fd.cFileName;
			_themes.push_back(td);
		} while( FindNextFile(hSearch, &fd) );
		FindClose(hSearch);
	}
	SetCurrentDirectory(curdir);
}

ThemeManager::~ThemeManager()
{
}

size_t ThemeManager::GetThemeCount()
{
	return _themes.size() + 1;
}

size_t ThemeManager::FindTheme(const string_t &name)
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

string_t ThemeManager::GetThemeName(size_t index)
{
	if( 0 == index )
		return "<standard>";
	return _themes[index-1].fileName.substr(
		0, _themes[index-1].fileName.size() - 4); // throw off the extension
}

bool ThemeManager::ApplyTheme(size_t index)
{
	bool res = (g_texman->LoadPackage(FILE_TEXTURES) > 0);
	if( index > 0 )
	{
		string_t filename = DIR_THEMES;
		filename += "\\";
		filename += _themes[index-1].fileName;
		res = res && (g_texman->LoadPackage(filename) > 0);
	}

	FOREACH( g_level->GetList(LIST_objects), GC_Object, object )
	{
		GC_2dSprite *pSprite = dynamic_cast<GC_2dSprite*>(object);
		if( pSprite && !pSprite->IsKilled() )
		{
			pSprite->UpdateTexture();
		}
	}

	return res;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
