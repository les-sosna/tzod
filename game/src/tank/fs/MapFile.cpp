// MapFile.cpp

#include "MapFile.h"
#include "FileSystem.h"

//////////////////////////////////////////////////////////

bool MapFile::_read_chunk_header(ChunkHeader &chdr)
{
	assert(!_modeWrite);
    switch (_file->Read(&chdr, sizeof(ChunkHeader)))
    {
        case FS::EC_OK:
            return true;
        case FS::EC_EOF:
            return false;
        default:
            throw std::runtime_error("file read error");
    }
}

void MapFile::_skip_block(size_t size)
{
	assert(!_modeWrite);
	_file->Seek(size, SEEK_CUR);
}

//////////////////////////////////////////////////////////

MapFile::MapFile(const SafePtr<FS::Stream> &file, bool write)
  : _file(file)
  , _modeWrite(write)
  , _headerWritten(false)
  , _isNewClass(true)
  , _obj_type(-1)
{
	if( !write )
	{
		ChunkHeader ch;

		_read_chunk_header(ch);
		if( CHUNK_HEADER_OPEN != ch.chunkType )
			throw std::runtime_error("invalid file");

		do
		{
			_read_chunk_header(ch);

			if( CHUNK_ATTRIB == ch.chunkType )
			{
				int type;
				ReadInt(type); // read attribute type


				//
				// check whether type is supported
				//

				bool supported_type = false;

				switch( type )
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
					std::string name;
					ReadString(name); // read attribute name

					std::string  value_str;
					int       value_int;
					float     value_float;

					switch( type )
					{
					case DATATYPE_INT:
						ReadInt(value_int);
						setMapAttribute(name, value_int);
						break;
					case DATATYPE_FLOAT:
						ReadFloat(value_float);
						setMapAttribute(name, value_float);
						break;
					case DATATYPE_STRING:
						ReadString(value_str);
						setMapAttribute(name, value_str);
						break;
					default:
						assert(false);
					}
				}
				else
				{
					_skip_block(ch.chunkSize - sizeof(int));
				}
			}
			else
			{
				_skip_block(ch.chunkSize);
			}
		} while( CHUNK_HEADER_CLOSE != ch.chunkType );
	}
}

MapFile::~MapFile(void)
{
}

void MapFile::WriteHeader()
{
	assert(!_headerWritten);

	ChunkHeader ch;


	//
	// open header
	//

	ch.chunkType = CHUNK_HEADER_OPEN;
	ch.chunkSize = 0;
	_file->Write(&ch, sizeof(ChunkHeader));


	//
	// write attributes
	//

	ch.chunkType = CHUNK_ATTRIB;

	for( std::map<std::string, int>::iterator
		it = _mapAttrs.attrs_int.begin(); it != _mapAttrs.attrs_int.end(); ++it )
	{
		ch.chunkSize = it->first.length() + sizeof(unsigned short) + sizeof(int);
		_file->Write(&ch, sizeof(ChunkHeader));
		WriteInt(DATATYPE_INT);
		WriteString(it->first);
		WriteInt(it->second);
	}

	for( std::map<std::string, float>::iterator
		it = _mapAttrs.attrs_float.begin(); it != _mapAttrs.attrs_float.end(); ++it )
	{
		ch.chunkSize = it->first.length() + sizeof(unsigned short) + sizeof(float);
		_file->Write(&ch, sizeof(ChunkHeader));
		WriteInt(DATATYPE_FLOAT);
		WriteString(it->first);
		WriteFloat(it->second);
	}

	for( std::map<std::string, std::string>::iterator
		it = _mapAttrs.attrs_str.begin(); it != _mapAttrs.attrs_str.end(); ++it )
	{
		ch.chunkSize = it->first.length() + it->second.length() + sizeof(unsigned short) * 2;
		_file->Write(&ch, sizeof(ChunkHeader));
		WriteInt(DATATYPE_STRING);
		WriteString(it->first);
		WriteString(it->second);
	}


	//
	// close header
	//

	ch.chunkType = CHUNK_HEADER_CLOSE;
	ch.chunkSize = 0;
	_file->Write(&ch, sizeof(ChunkHeader));
}

void MapFile::WriteInt(int value)
{
	assert(_modeWrite);
    uint32_t tmp = value;
	_file->Write(&tmp, 4);
}

void MapFile::WriteFloat(float value)
{
	assert(_modeWrite);
	_file->Write(&value, sizeof(float));
}

void MapFile::WriteString(const std::string &value)
{
	assert(_modeWrite);
	assert(value.length() <= 0xffff);
	uint16_t len = (uint16_t) (value.length() & 0xffff);
	_file->Write(&len, 2);
    if( len )
        _file->Write(value.data(), len);
}

void MapFile::ReadInt(int &value)
{
	assert(!_modeWrite);
    int32_t tmp;
	if( FS::EC_OK != _file->Read(&tmp, 4) )
        throw std::runtime_error("unexpected end of file");
    value = tmp;
}

void MapFile::ReadFloat(float &value)
{
    static_assert(sizeof(value) == 4, "size of float is not 4");
	assert(!_modeWrite);
	if( FS::EC_OK != _file->Read(&value, 4) )
        throw std::runtime_error("unexpected end of file");
}

void MapFile::ReadString(std::string &value)
{
	assert(!_modeWrite);
	uint16_t len;
	if( FS::EC_OK != _file->Read(&len, 2) )
        throw std::runtime_error("unexpected end of file");
    value.resize(len);
	if( len )
        if( FS::EC_OK != _file->Read(&value[0], len) )
            throw std::runtime_error("unexpected end of file");
}

bool MapFile::getMapAttribute(const std::string &name, int &value) const
{
	std::map<std::string, int>::const_iterator it = _mapAttrs.attrs_int.find(name);
	if( _mapAttrs.attrs_int.end() != it )
	{
		value = it->second;
		return true;
	}
	return false;
}

bool MapFile::getMapAttribute(const std::string &name, float &value) const
{
	std::map<std::string, float>::const_iterator it = _mapAttrs.attrs_float.find(name);
	if( _mapAttrs.attrs_float.end() != it )
	{
		value = it->second;
		return true;
	}
	return false;
}

bool MapFile::getMapAttribute(const std::string &name, std::string &value) const
{
	std::map<std::string, std::string>::const_iterator it = _mapAttrs.attrs_str.find(name);
	if( _mapAttrs.attrs_str.end() != it )
	{
		value = it->second;
		return true;
	}
	return false;
}


void MapFile::setMapAttribute(const std::string &name, int value)
{
	assert(!_headerWritten);
	_mapAttrs.attrs_int[name] = value;
}

void MapFile::setMapAttribute(const std::string &name, float value)
{
	assert(!_headerWritten);
	_mapAttrs.attrs_float[name] = value;
}

void MapFile::setMapAttribute(const std::string &name, const std::string &value)
{
	assert(!_headerWritten);
	_mapAttrs.attrs_str[name] = value;
}



bool MapFile::getObjectAttribute(const std::string &name, int &value) const
{
	assert(!_modeWrite);
	std::map<std::string, int>::const_iterator it;
	it = _obj_attrs.attrs_int.find(name);
	if( _obj_attrs.attrs_int.end() == it )
		return false;
	value = it->second;
	return true;
}

bool MapFile::getObjectAttribute(const std::string &name, float &value) const
{
	assert(!_modeWrite);
	std::map<std::string, float>::const_iterator it;
	it = _obj_attrs.attrs_float.find(name);
	if( _obj_attrs.attrs_float.end() == it )
		return false;
	value = it->second;
	return true;
}

bool MapFile::getObjectAttribute(const std::string &name, std::string &value) const
{
	assert(!_modeWrite);
	std::map<std::string, std::string>::const_iterator it;
	it = _obj_attrs.attrs_str.find(name);
	if( _obj_attrs.attrs_str.end() == it )
		return false;
	value = it->second;
	return true;
}

void MapFile::setObjectAttribute(const std::string &name, int value)
{
	if( _isNewClass )
	{
#ifdef _DEBUG
		// check that given name is unique
		for( size_t i = 0; i < _managed_classes.back()._propertyset.size(); i++ )
			assert(_managed_classes.back()._propertyset[i].name != name);
#endif

		ObjectDefinition::Property p;
		p.type = DATATYPE_INT;
		p.name = name;
		_managed_classes.back()._propertyset.push_back(p);
	}
	_buffer.write((const char*) &value, sizeof(int));
}

void MapFile::setObjectAttribute(const std::string &name, float value)
{
	if( _isNewClass )
	{
#ifdef _DEBUG
		// check that given name is unique
		for( size_t i = 0; i < _managed_classes.back()._propertyset.size(); i++ )
			assert(_managed_classes.back()._propertyset[i].name != name);
#endif

		ObjectDefinition::Property p;
		p.type = DATATYPE_FLOAT;
		p.name = name;
		_managed_classes.back()._propertyset.push_back(p);
	}
	_buffer.write((const char*) &value, sizeof(float));
}

void MapFile::setObjectAttribute(const std::string &name, const std::string &value)
{
	if( _isNewClass )
	{
#ifdef _DEBUG
		// check that given name is unique
		for( size_t i = 0; i < _managed_classes.back()._propertyset.size(); i++ )
			assert(_managed_classes.back()._propertyset[i].name != name);
#endif

		ObjectDefinition::Property p;
		p.type = DATATYPE_STRING;
		p.name = name;
		_managed_classes.back()._propertyset.push_back(p);
	}
	uint16_t len = (uint16_t) (value.length() & 0xffff);
	_buffer.write((const char*) &len, 2);
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

void MapFile::setObjectDefault(const char *cls, const char *attr, const std::string &value)
{
	_defaults[cls].attrs_str[attr] = value;
}

const std::string& MapFile::GetCurrentClassName() const
{
	assert(!_modeWrite);
	assert(_obj_type >= 0 && _obj_type < _managed_classes.size());
    return _managed_classes[_obj_type]._className;
}

void MapFile::BeginObject(const char *classname)
{
	assert(_modeWrite);

	if( !_headerWritten )
	{
		WriteHeader();
		_headerWritten = true;
	}
	

	_buffer.str(""); // clear buffer


	//
	// check that class is known
	//

	int32_t obj_type;

	std::map<std::string, size_t>::iterator it;
	it = _name_to_index.find(classname);
	if( _name_to_index.end() == it )
	{
		_isNewClass = true;
		_name_to_index[classname] = _managed_classes.size();
		obj_type = (int32_t) _managed_classes.size();
		_managed_classes.push_back(ObjectDefinition());
		_managed_classes.back()._className = classname;
	}
	else
	{
		obj_type = (int32_t) it->second;
		_isNewClass = false;
	}

	_buffer.write((const char*) &obj_type, sizeof(int32_t));
}

void MapFile::WriteCurrentObject()
{
	assert(_modeWrite);

	ChunkHeader ch;


	//
	// writing class definition
	//
	if( _isNewClass )
	{
		const ObjectDefinition &od = _managed_classes.back();
		ch.chunkType = CHUNK_OBJDEF;
		ch.chunkSize = od.CalcSize();
		_file->Write(&ch, sizeof(ChunkHeader));
		WriteString(od._className);
		WriteInt((int)od._propertyset.size());
		for( size_t i = 0; i < od._propertyset.size(); i++ )
		{
			WriteInt(od._propertyset[i].type);
			WriteString(od._propertyset[i].name);
		}
	}

	//
	// writing buffered data
	//
	std::string str(_buffer.str());
	ch.chunkType = CHUNK_OBJECT;
	ch.chunkSize = str.size();
	assert(ch.chunkSize > 0);
	_file->Write(&ch, sizeof(ChunkHeader));
	_file->Write(str.data(), ch.chunkSize);
}

bool MapFile::NextObject()
{
	assert(!_modeWrite);

	for( ChunkHeader ch; _read_chunk_header(ch); )
	{
		switch( ch.chunkType )
		{
			case CHUNK_OBJDEF:
			{
				_managed_classes.push_back(ObjectDefinition());
				ObjectDefinition &od = _managed_classes.back();
				ReadString(od._className);
				int propertyCount;
				ReadInt(propertyCount);
				od._propertyset.resize(propertyCount);
				for( int i = 0; i < propertyCount; i++ )
				{
					ReadInt(reinterpret_cast<int&>(od._propertyset[i].type));
					ReadString(od._propertyset[i].name);
				}
				break;
			}

			case CHUNK_OBJECT:
			{
				_obj_attrs.clear();

				ReadInt(_obj_type);
				if( _obj_type < 0 || _obj_type >= _managed_classes.size() )
					throw std::runtime_error("invalid class");

				const std::vector<ObjectDefinition::Property> &ps =
					_managed_classes[_obj_type]._propertyset;

				for( size_t i = 0; i < ps.size(); i++ )
				{
					switch( ps[i].type )
					{
					case DATATYPE_INT:
						ReadInt(_obj_attrs.attrs_int[ps[i].name]);
						break;
					case DATATYPE_FLOAT:
						ReadFloat(_obj_attrs.attrs_float[ps[i].name]);
						break;
					case DATATYPE_STRING:
						ReadString(_obj_attrs.attrs_str[ps[i].name]);
						break;
					default:
						throw std::runtime_error("invalid file");
					}
				}
				return true;
			}

			default:
				// skip everything we don't understand
				_skip_block(ch.chunkSize);
		}
	}

	return false;
}

bool MapFile::loading() const
{
	return !_modeWrite;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
