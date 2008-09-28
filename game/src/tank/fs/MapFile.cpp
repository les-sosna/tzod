// MapFile.cpp

/*
#define _CRT_SECURE_NO_DEPRECATE

#ifdef _WIN32
#include <crtdbg.h>
#else
#undef _DEBUG
#define _ASSERT
#endif
*/

#include "stdafx.h"
#include "MapFile.h"

//////////////////////////////////////////////////////////

bool MapFile::_read_chunk_header(ChunkHeader &chdr)
{
	_ASSERT(is_open() && !_modeWrite);
	return (1 == fread(&chdr, sizeof(ChunkHeader), 1, _file));
}

bool MapFile::_write_chunk_header(const ChunkHeader &chdr)
{
	_ASSERT(is_open() && _modeWrite);
	return (1 == fwrite(&chdr, sizeof(ChunkHeader), 1, _file));
}

bool MapFile::_skip_block(size_t size)
{
	_ASSERT(is_open() && !_modeWrite);
	return (0 == fseek(_file, (long) size, SEEK_CUR));
}

//////////////////////////////////////////////////////////

MapFile::MapFile(void)
{
	_file        = NULL;
	_modeWrite   = false;
	_isNewClass  = true;
	_obj_type    = -1;
}

MapFile::~MapFile(void)
{
	if( is_open() ) Close();
}

bool MapFile::Open(const string_t &filename, bool write)
{
	_ASSERT(!is_open());

	_file = fopen(filename.c_str(), write ? "wb" : "rb");

	if( !_file )
		return false;


	_modeWrite = write;

    try
	{
		if( write )
		{
			ChunkHeader ch;


			//
			// write file header
			//

			ch.chunkType = CHUNK_HEADER_OPEN;
			ch.chunkSize = 0;
			if( !_write_chunk_header(ch) )
				throw false;


			//
			// write attributes
			//

			ch.chunkType = CHUNK_ATTRIB;

			for( std::map<string_t, int>::iterator
				it = _mapAttrs.attrs_int.begin(); it != _mapAttrs.attrs_int.end(); ++it )
			{
				ch.chunkSize = it->first.length() + sizeof(unsigned short) + sizeof(int);
				if( !_write_chunk_header(ch) )
					throw false;
				WriteInt(DATATYPE_INT);
				WriteString(it->first);
				WriteInt(it->second);
			}

			for( std::map<string_t, float>::iterator
				it = _mapAttrs.attrs_float.begin(); it != _mapAttrs.attrs_float.end(); ++it )
			{
				ch.chunkSize = it->first.length() + sizeof(unsigned short) + sizeof(float);
				if( !_write_chunk_header(ch) )
					throw false;
				WriteInt(DATATYPE_FLOAT);
				WriteString(it->first);
				WriteFloat(it->second);
			}

			for( std::map<string_t, string_t>::iterator
				it = _mapAttrs.attrs_str.begin(); it != _mapAttrs.attrs_str.end(); ++it )
			{
				ch.chunkSize = it->first.length() + it->second.length() + sizeof(unsigned short) * 2;
				if( !_write_chunk_header(ch) )
					throw false;
                WriteInt(DATATYPE_STRING);
				WriteString(it->first);
				WriteString(it->second);
			}


			//
			// close header
			//

			ch.chunkType = CHUNK_HEADER_CLOSE;
			ch.chunkSize = 0;
			if( !_write_chunk_header(ch) )
				throw false;
		}
		else
		{
			ChunkHeader ch;

			if( !_read_chunk_header(ch) || CHUNK_HEADER_OPEN != ch.chunkType )
				throw false;

			_mapAttrs.clear();

			do
			{
				if( !_read_chunk_header(ch) )
					throw false;

				if( CHUNK_ATTRIB == ch.chunkType )
				{
					int type;
					if( !ReadInt(type) ) // read attribute type
						throw false;


					//
					// check whether type is supported
					//

					bool supported_type = false;

					switch(type)
					{
					case DATATYPE_INT:
					case DATATYPE_FLOAT:
					case DATATYPE_STRING:
						supported_type = true;
						break;
					};


					//
					// read attribute's name and value
					//

					if( supported_type )
					{
						string_t name;
						if( !ReadString(name) ) // read attribute name
							throw false;

						string_t  value_str;
						int       value_int;
						float     value_float;

						switch(type)
						{
						case DATATYPE_INT:
							if( !ReadInt(value_int) ) throw false;
							setMapAttribute(name, value_int);
							break;
						case DATATYPE_FLOAT:
							if( !ReadFloat(value_float) ) throw false;
							setMapAttribute(name, value_float);
							break;
						case DATATYPE_STRING:
							if( !ReadString(value_str) ) throw false;
							setMapAttribute(name, value_str);
							break;
						default:
							_ASSERT(0);
						};
					}
					else if( !_skip_block(ch.chunkSize - sizeof(int)) )
					{
						throw false;
					}
				}
				else
				{
					_skip_block(ch.chunkSize);
				}
			} while( CHUNK_HEADER_CLOSE != ch.chunkType );
		}
	}
	catch(bool)
	{
		Close();
		return false;
	}

	return true;
}

bool MapFile::WriteInt(int value)
{
	_ASSERT(is_open() && _modeWrite);
	return (1 == fwrite(&value, sizeof(int), 1, _file));
}

bool MapFile::WriteFloat(float value)
{
	_ASSERT(is_open() && _modeWrite);
	return (1 == fwrite(&value, sizeof(float), 1, _file));
}

bool MapFile::WriteString(const string_t &value)
{
	_ASSERT(is_open() && _modeWrite);
	_ASSERT(value.length() <= 0xffff);
    unsigned short len = (unsigned short) (value.length() & 0xffff);
	if( 1 != fwrite(&len, sizeof(unsigned short), 1, _file) )
		return false;
	if( 0 == len ) return true;
    return (len == fwrite(&(*value.begin()), sizeof(char), len, _file));
}

bool MapFile::ReadInt(int &value)
{
	_ASSERT(is_open() && !_modeWrite);
	return (1 == fread(&value, sizeof(int), 1, _file));
}

bool MapFile::ReadFloat(float &value)
{
	_ASSERT(is_open() && !_modeWrite);
	return (1 == fread(&value, sizeof(float), 1, _file));
}

bool MapFile::ReadString(string_t &value)
{
	_ASSERT(is_open() && !_modeWrite);
    unsigned short len;
	if( 1 != fread(&len, sizeof(unsigned short), 1, _file) )
		return false;
	std::vector<char> tmp(len+1);
	bool result = (len == fread(&*tmp.begin(), sizeof(char), len, _file));
	tmp[len] = 0;
	value = &*tmp.begin();
    return result;
}

bool MapFile::getMapAttribute(const string_t &name, int &value) const
{
	std::map<string_t, int>::const_iterator it = _mapAttrs.attrs_int.find(name);
	if( _mapAttrs.attrs_int.end() != it )
	{
		value = it->second;
		return true;
	}
	return false;
}

bool MapFile::getMapAttribute(const string_t &name, float &value) const
{
	std::map<string_t, float>::const_iterator it = _mapAttrs.attrs_float.find(name);
	if( _mapAttrs.attrs_float.end() != it )
	{
		value = it->second;
		return true;
	}
	return false;
}

bool MapFile::getMapAttribute(const string_t &name, string_t &value) const
{
	std::map<string_t, string_t>::const_iterator it = _mapAttrs.attrs_str.find(name);
	if( _mapAttrs.attrs_str.end() != it )
	{
		value = it->second;
		return true;
	}
	return false;
}


void MapFile::setMapAttribute(const string_t &name, int value)
{
	_mapAttrs.attrs_int[name] = value;
}

void MapFile::setMapAttribute(const string_t &name, float value)
{
	_mapAttrs.attrs_float[name] = value;
}

void MapFile::setMapAttribute(const string_t &name, const string_t &value)
{
	_mapAttrs.attrs_str[name] = value;
}



bool MapFile::getObjectAttribute(const string_t &name, int &value) const
{
	_ASSERT(is_open() && !_modeWrite);
	std::map<string_t, int>::const_iterator it;
	it = _obj_attrs.attrs_int.find(name);
	if( _obj_attrs.attrs_int.end() == it )
		return false;
	value = it->second;
	return true;
}

bool MapFile::getObjectAttribute(const string_t &name, float &value) const
{
	_ASSERT(is_open() && !_modeWrite);
	std::map<string_t, float>::const_iterator it;
	it = _obj_attrs.attrs_float.find(name);
	if( _obj_attrs.attrs_float.end() == it )
		return false;
	value = it->second;
	return true;
}

bool MapFile::getObjectAttribute(const string_t &name, string_t &value) const
{
	_ASSERT(is_open() && !_modeWrite);
	std::map<string_t, string_t>::const_iterator it;
	it = _obj_attrs.attrs_str.find(name);
	if( _obj_attrs.attrs_str.end() == it )
		return false;
	value = it->second;
	return true;
}

void MapFile::setObjectAttribute(const string_t &name, int value)
{
	if( _isNewClass )
	{
#ifdef _DEBUG
		// check that given name is unique
		for( size_t i = 0; i < _managed_classes.back()._propertyset.size(); i++ )
			_ASSERT(_managed_classes.back()._propertyset[i].name != name);
#endif

		ObjectDefinition::Property p;
		p.type = DATATYPE_INT;
		p.name = name;
		_managed_classes.back()._propertyset.push_back(p);
	}
	_buffer.write((const char*) &value, sizeof(int));
}

void MapFile::setObjectAttribute(const string_t &name, float value)
{
	if( _isNewClass )
	{
#ifdef _DEBUG
		// check that given name is unique
		for( size_t i = 0; i < _managed_classes.back()._propertyset.size(); i++ )
			_ASSERT(_managed_classes.back()._propertyset[i].name != name);
#endif

		ObjectDefinition::Property p;
		p.type = DATATYPE_FLOAT;
		p.name = name;
		_managed_classes.back()._propertyset.push_back(p);
	}
	_buffer.write((const char*) &value, sizeof(float));
}

void MapFile::setObjectAttribute(const string_t &name, const string_t &value)
{
	if( _isNewClass )
	{
#ifdef _DEBUG
		// check that given name is unique
		for( size_t i = 0; i < _managed_classes.back()._propertyset.size(); i++ )
			_ASSERT(_managed_classes.back()._propertyset[i].name != name);
#endif

		ObjectDefinition::Property p;
		p.type = DATATYPE_STRING;
		p.name = name;
		_managed_classes.back()._propertyset.push_back(p);
	}
	unsigned short len = (unsigned short) (value.length() & 0xffff);
	_buffer.write((const char*) &len, sizeof(unsigned short));
	_buffer.write(value.data(), (std::streamsize) value.length());
}

void MapFile::setObjectDefault(const char *cls, const char *attr, int value)
{
	_defaults[cls].attrs_int[attr] = value;
}

void MapFile::setObjectDefault(const char *cls, const char *attr, float value)
{
	_defaults[cls].attrs_float[attr] = value;
}

void MapFile::setObjectDefault(const char *cls, const char *attr, const string_t &value)
{
	_defaults[cls].attrs_str[attr] = value;
}

const string_t& MapFile::GetCurrentClassName() const
{
	_ASSERT(is_open() && !_modeWrite);
	_ASSERT(_obj_type < _managed_classes.size());
    return _managed_classes[_obj_type]._className;
}

void MapFile::BeginObject(const char *classname)
{
	_ASSERT(is_open() && _modeWrite);

	_buffer.str(""); // clear buffer


	//
	// check that class is known
	//

	int obj_type;

	std::map<string_t, size_t>::iterator it;
	it = _name_to_index.find(classname);
	if( _name_to_index.end() == it )
	{
		_isNewClass = true;
		_name_to_index[classname] = _managed_classes.size();
		obj_type = (int) _managed_classes.size();
		_managed_classes.push_back(ObjectDefinition());
		_managed_classes.back()._className = classname;
	}
	else
	{
		obj_type = (int) it->second;
		_isNewClass = false;
	}

	_buffer.write((const char*) &obj_type, sizeof(int));
}

bool MapFile::WriteCurrentObject()
{
	_ASSERT(is_open() && _modeWrite);

	ChunkHeader ch;


	//
	// writing class definition
	//
	if( _isNewClass )
	{
		const ObjectDefinition &od = _managed_classes.back();
		ch.chunkType = CHUNK_OBJDEF;
		ch.chunkSize = od.CalcSize();
		_write_chunk_header(ch);
		if( !WriteString(od._className) ||
			!WriteInt((int)od._propertyset.size()) )
		{
			return false;
		}
		for( size_t i = 0; i < od._propertyset.size(); i++ )
		{
			if( !WriteInt(od._propertyset[i].type) ||
				!WriteString(od._propertyset[i].name) )
			{
				return false;
			}
		}
	}

	//
	// writing buffered data
	//
	string_t str(_buffer.str());
	ch.chunkType = CHUNK_OBJECT;
	ch.chunkSize = str.size();
	_ASSERT(ch.chunkSize > 0);
	_write_chunk_header(ch);
	return (ch.chunkSize == fwrite(str.data(), sizeof(char), ch.chunkSize, _file));
}

bool MapFile::NextObject()
{
	_ASSERT(is_open() && !_modeWrite);

	ChunkHeader ch;
	while( _read_chunk_header(ch) )
	{
		switch(ch.chunkType)
		{
			case CHUNK_OBJDEF:
			{
				_managed_classes.push_back(ObjectDefinition());
				ObjectDefinition &od = _managed_classes.back();
				if( !ReadString(od._className) ) return false;
				int property_count;
				if( !ReadInt(property_count) ) return false;
				od._propertyset.resize(property_count);
				for( int i = 0; i < property_count; i++ )
				{
					if( !ReadInt(reinterpret_cast<int&>(od._propertyset[i].type)) )
						return false;
					if( !ReadString(od._propertyset[i].name) ) return false;
				}
			} break;

			case CHUNK_OBJECT:
			{
				_obj_attrs.clear();

				if( !ReadInt(reinterpret_cast<int&>(_obj_type)) )
					return false;
                if( _obj_type < _managed_classes.size() )
				{
					const std::vector<ObjectDefinition::Property> &ps =
						_managed_classes[_obj_type]._propertyset;

					for( size_t i = 0; i < ps.size(); i++ )
					{
						switch( ps[i].type )
						{
						case DATATYPE_INT:
							if( !ReadInt(_obj_attrs.attrs_int[ps[i].name]) )
								return false;
							break;
						case DATATYPE_FLOAT:
							if( !ReadFloat(_obj_attrs.attrs_float[ps[i].name]) )
								return false;
							break;
						case DATATYPE_STRING:
							if( !ReadString(_obj_attrs.attrs_str[ps[i].name]) )
								return false;
							break;
						default:
							_ASSERT(0);
							return false;
						}
					}
                    return true;
				}
				return false;
			} break;

			default:
				_skip_block(ch.chunkSize);
		}
	}

	return false;
}

bool MapFile::Close()
{
	_ASSERT(is_open());

    bool res = (0 == fclose(_file));
	_file = NULL;
	return res;
}

bool MapFile::is_open() const
{
	return (NULL != _file);
}

bool MapFile::loading() const
{
	return !_modeWrite;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
