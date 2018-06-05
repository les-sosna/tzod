#include "inc/MapFile.h"
#include <fs/FileSystem.h>
#include <algorithm>
#include <cassert>
#include <cstring>

bool MapFile::_read_chunk_header(ChunkHeader &chdr)
{
	assert(!_modeWrite);
	return 1 == _file.Read(&chdr, sizeof(ChunkHeader), 1);
}

void MapFile::_skip_block(size_t size)
{
	assert(!_modeWrite);
	_file.Seek(size, SEEK_CUR);
}

static const unsigned int MAX_BUFFER_SIZE = 0x10000;

MapFile::MapFile(FS::Stream &stream, bool write)
	: _file(stream)
	, _buffer(write ? new char[MAX_BUFFER_SIZE] : nullptr)
	, _modeWrite(write)
	, _headerWritten(false)
	, _isNewClass(true)
	, _objType(-1)
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

MapFile::~MapFile()
{
}

void MapFile::WriteHeader()
{
	assert(!_headerWritten);

	ChunkHeader ch;

	// open header
	ch.chunkType = CHUNK_HEADER_OPEN;
	ch.chunkSize = 0;
	GetWriteBuffer<ChunkHeader>() = ch;

	// write map attributes
	ch.chunkType = CHUNK_ATTRIB;

	for( auto &attr: _mapAttrs.attrs_int )
	{
		ch.chunkSize = static_cast<uint32_t>(attr.first.length()) + sizeof(unsigned short) + sizeof(int);
		GetWriteBuffer<ChunkHeader>() = ch;
		WriteInt(DATATYPE_INT);
		WriteString(attr.first);
		WriteInt(attr.second);
	}

	for( auto &attr: _mapAttrs.attrs_float )
	{
		ch.chunkSize = static_cast<uint32_t>(attr.first.length()) + sizeof(unsigned short) + sizeof(float);
		GetWriteBuffer<ChunkHeader>() = ch;
		WriteInt(DATATYPE_FLOAT);
		WriteString(attr.first);
		WriteFloat(attr.second);
	}

	for( auto &attr: _mapAttrs.attrs_str )
	{
		ch.chunkSize = static_cast<uint32_t>(attr.first.length() + attr.second.length()) + sizeof(unsigned short) * 2;
		GetWriteBuffer<ChunkHeader>() = ch;
		WriteInt(DATATYPE_STRING);
		WriteString(attr.first);
		WriteString(attr.second);
	}

	// close header
	ch.chunkType = CHUNK_HEADER_CLOSE;
	ch.chunkSize = 0;
	GetWriteBuffer<ChunkHeader>() = ch;
}

void MapFile::WriteInt(int value)
{
	assert(_modeWrite);
	GetWriteBuffer<int32_t>() = value;
}

void MapFile::WriteFloat(float value)
{
	assert(_modeWrite);
	GetWriteBuffer<float>() = value;
}

void MapFile::WriteString(std::string_view value)
{
	assert(_modeWrite);
	assert(value.length() <= 0xffff);
	GetWriteBuffer<uint16_t>() = (uint16_t)(value.length() & 0xffff);
	if (!value.empty())
		WriteData(value.data(), value.length());
}

void MapFile::WriteData(const void *data, size_t size)
{
	assert(_modeWrite);
	memcpy(GetWriteBuffer(size), data, size);
}

inline void* MapFile::GetWriteBuffer(size_t size)
{
	assert(size <= MAX_BUFFER_SIZE);
	if (_bufferSize + size > MAX_BUFFER_SIZE)
	{
		_file.Write(_buffer.get(), _bufferSize);
		_bufferSize = 0;
	}
	void *result = _buffer.get() + _bufferSize;
	_bufferSize += size;
	return result;
}

void MapFile::ReadInt(int &value)
{
	assert(!_modeWrite);
	int32_t tmp;
	if( 1 != _file.Read(&tmp, 4, 1) )
		throw std::runtime_error("unexpected end of file");
	value = tmp;
}

void MapFile::ReadFloat(float &value)
{
	static_assert(sizeof(value) == 4, "size of float is not 4");
	assert(!_modeWrite);
	if( 1 != _file.Read(&value, 4, 1) )
		throw std::runtime_error("unexpected end of file");
}

void MapFile::ReadString(std::string &value)
{
	assert(!_modeWrite);
	uint16_t len;
	if( 1 != _file.Read(&len, 2, 1) )
		throw std::runtime_error("unexpected end of file");
	value.resize(len);
	if( len )
		if( 1 != _file.Read(&value[0], len, 1) )
			throw std::runtime_error("unexpected end of file");
}

bool MapFile::getMapAttribute(std::string_view name, int &value) const
{
	auto it = _mapAttrs.attrs_int.find(name);
	if( _mapAttrs.attrs_int.end() != it )
	{
		value = it->second;
		return true;
	}
	return false;
}

bool MapFile::getMapAttribute(std::string_view name, float &value) const
{
	auto it = _mapAttrs.attrs_float.find(name);
	if( _mapAttrs.attrs_float.end() != it )
	{
		value = it->second;
		return true;
	}
	return false;
}

bool MapFile::getMapAttribute(std::string_view name, std::string &value) const
{
	auto it = _mapAttrs.attrs_str.find(name);
	if( _mapAttrs.attrs_str.end() != it )
	{
		value = it->second;
		return true;
	}
	return false;
}


void MapFile::setMapAttribute(std::string name, int value)
{
	assert(!_headerWritten);
	_mapAttrs.attrs_int[std::move(name)] = value;
}

void MapFile::setMapAttribute(std::string name, float value)
{
	assert(!_headerWritten);
	_mapAttrs.attrs_float[std::move(name)] = value;
}

void MapFile::setMapAttribute(std::string name, std::string value)
{
	assert(!_headerWritten);
	_mapAttrs.attrs_str[std::move(name)] = std::move(value);
}

bool MapFile::getObjectAttribute(std::string_view name, int &value) const
{
	assert(!_modeWrite);
	auto &pd = _managed_classes[_objType].propertyDefinitions;
	auto it = std::find_if(begin(pd), end(pd), [&](auto &p) { return p.name == name; });
	if (it != pd.end())
	{
		value = it->value_int;
		return true;
	}
	return false;
}

bool MapFile::getObjectAttribute(std::string_view name, float &value) const
{
	assert(!_modeWrite);
	auto &pd = _managed_classes[_objType].propertyDefinitions;
	auto it = std::find_if(begin(pd), end(pd), [&](auto &p) { return p.name == name; });
	if (it != pd.end())
	{
		value = it->value_float;
		return true;
	}
	return false;
}

bool MapFile::getObjectAttribute(std::string_view name, std::string &value) const
{
	assert(!_modeWrite);
	auto &pd = _managed_classes[_objType].propertyDefinitions;
	auto it = std::find_if(begin(pd), end(pd), [&](auto &p) { return p.name == name; });
	if (it != pd.end())
	{
		value = it->value_string;
		return true;
	}
	return false;
}

void MapFile::setObjectAttribute(std::string_view name, int value)
{
	if( _isNewClass )
	{
#ifndef NDEBUG
		// check that given name is unique
		for( size_t i = 0; i < _managed_classes.back().propertyDefinitions.size(); i++ )
			assert(_managed_classes.back().propertyDefinitions[i].name != name);
#endif

		ObjectDefinition::Property p(DATATYPE_INT);
		p.name = name;
		_managed_classes.back().propertyDefinitions.push_back(std::move(p));
	}
	_managed_classes[_objType].propertyDefinitions[_numProperties++].value_int = value;
}

void MapFile::setObjectAttribute(std::string_view name, float value)
{
	if( _isNewClass )
	{
#ifdef _DEBUG
		// check that given name is unique
		for( size_t i = 0; i < _managed_classes.back().propertyDefinitions.size(); i++ )
			assert(_managed_classes.back().propertyDefinitions[i].name != name);
#endif

		ObjectDefinition::Property p(DATATYPE_FLOAT);
		p.name = name;
		_managed_classes.back().propertyDefinitions.push_back(std::move(p));
	}
	_managed_classes[_objType].propertyDefinitions[_numProperties++].value_float = value;
}

void MapFile::setObjectAttribute(std::string_view name, std::string_view value)
{
	if( _isNewClass )
	{
#ifdef _DEBUG
		// check that given name is unique
		for( size_t i = 0; i < _managed_classes.back().propertyDefinitions.size(); i++ )
			assert(_managed_classes.back().propertyDefinitions[i].name != name);
#endif

		ObjectDefinition::Property p(DATATYPE_STRING);
		p.name = name;
		_managed_classes.back().propertyDefinitions.push_back(std::move(p));
	}

	_managed_classes[_objType].propertyDefinitions[_numProperties++].value_string = value;
}

std::string_view MapFile::GetCurrentClassName() const
{
	assert(!_modeWrite);
	assert(_objType >= 0 && _objType < (int) _managed_classes.size());
	return _managed_classes[_objType].className;
}

void MapFile::BeginObject(std::string_view classname)
{
	assert(_modeWrite);

	if( !_headerWritten )
	{
		WriteHeader();
		_headerWritten = true;
	}

	auto it = _name_to_index.find(classname);
	if( _name_to_index.end() == it )
	{
		_isNewClass = true;
		_objType = (int)_managed_classes.size();
		_name_to_index.emplace(classname, _objType);
		_managed_classes.emplace_back();
		_managed_classes.back().className = classname;
	}
	else
	{
		_isNewClass = false;
		_objType = it->second;
	}

	_numProperties = 0;
}

void MapFile::WriteCurrentObject()
{
	assert(_modeWrite);
	assert(_numProperties == _managed_classes[_objType].propertyDefinitions.size());

	ChunkHeader ch;

	// object class
	if( _isNewClass )
	{
		const ObjectDefinition &od = _managed_classes.back();
		ch.chunkType = CHUNK_OBJDEF;
		ch.chunkSize = 0;
		GetWriteBuffer<ChunkHeader>() = ch;
		WriteString(od.className);
		WriteInt((int)od.propertyDefinitions.size());
		for( auto &prop: od.propertyDefinitions )
		{
			WriteInt(prop._type);
			WriteString(prop.name);
		}
	}

	// object data
	ch.chunkType = CHUNK_OBJECT;
	ch.chunkSize = 0;
	GetWriteBuffer<ChunkHeader>() = ch;

	WriteInt(_objType);

	for (auto &prop : _managed_classes[_objType].propertyDefinitions)
	{
		switch (prop._type)
		{
		case DATATYPE_INT:
			WriteInt(prop.value_int);
			break;
		case DATATYPE_FLOAT:
			WriteFloat(prop.value_float);
			break;
		case DATATYPE_STRING:
			WriteString(prop.value_string);
			break;
		default:
			assert(false);
		}
	}
}

void MapFile::WriteEndOfFile()
{
	assert(_modeWrite);
	_file.Write(_buffer.get(), _bufferSize);
	_bufferSize = 0;
}

bool MapFile::ReadNextObject()
{
	assert(!_modeWrite);

	for( ChunkHeader ch; _read_chunk_header(ch); )
	{
		switch( ch.chunkType )
		{
			case CHUNK_OBJDEF:
			{
				_managed_classes.emplace_back();
				ObjectDefinition &od = _managed_classes.back();
				ReadString(od.className);
				int propertyCount;
				ReadInt(propertyCount);
				od.propertyDefinitions.reserve(propertyCount);
				for (int i = 0; i < propertyCount; i++)
				{
					int propType;
					ReadInt(propType);
					auto &prop = od.propertyDefinitions.emplace_back(static_cast<enumDataTypes>(propType));
					ReadString(prop.name);
				}
				break;
			}

			case CHUNK_OBJECT:
			{
				ReadInt(_objType);
				if( _objType < 0 || _objType >= (int) _managed_classes.size() )
					throw std::runtime_error("invalid class");

				for( auto &prop: _managed_classes[_objType].propertyDefinitions )
				{
					switch(prop._type)
					{
					case DATATYPE_INT:
						ReadInt(prop.value_int);
						break;
					case DATATYPE_FLOAT:
						ReadFloat(prop.value_float);
						break;
					case DATATYPE_STRING:
						ReadString(prop.value_string);
						break;
					default:
						assert(false);
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
