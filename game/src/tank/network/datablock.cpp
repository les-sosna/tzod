// datablock.cpp
///////////////////////////////////////////////////

#include "stdafx.h"
#include "datablock.h"


////////////////////////////////////////////////////////////////

template<>
DataBlock DataWrap<std::string>(std::string &data, DataBlock::type_type type)
{
	DataBlock db(data.length() + 1); // include '\0'
	db.type() = type;
	memcpy(db.Data(), data.c_str(), db.DataSize());
	return db;
}

template<>
DataBlock DataWrap<const std::string>(const std::string &data, DataBlock::type_type type)
{
	DataBlock db(data.length() + 1); // include '\0'
	db.type() = type;
	memcpy(db.Data(), data.c_str(), db.DataSize());
	return db;
}

////////////////////////////////////////////////////////////////

DataBlock::DataBlock()
{
	_data = malloc(sizeof(Header));
	h().type = DBTYPE_UNKNOWN;
	h().rawSize = sizeof(Header);
}

DataBlock::DataBlock(size_t dataSize)
{
	_ASSERT((size_t) std::numeric_limits<size_type>::max() > dataSize + sizeof(Header));
	_data = malloc(dataSize + sizeof(Header));
	h().rawSize = dataSize + sizeof(Header);
	h().type = DBTYPE_UNKNOWN;
}

DataBlock::DataBlock(const DataBlock &src)
{
	_data = malloc(src.h().rawSize);
	memcpy(_data, src._data, src.h().rawSize);
}

DataBlock::~DataBlock()
{
	_ASSERT(_data);
	free(_data);
}

DataBlock& DataBlock::operator=(const DataBlock &src)
{
	_ASSERT(_data);
	if( src.RawSize() != RawSize() )
	{
		free(_data);
		_data = malloc(src.RawSize());
	}
	memcpy(_data, src.RawData(), src.RawSize());
	return *this;
}

size_t DataBlock::Parse(void *data, size_t size)
{
	_ASSERT(_data);

	if( size < sizeof(Header) )
		return false;

	size_t rs = ((Header *) data)->rawSize;
	if( size < rs )
		return false;

	if( RawSize() != rs )
	{
		free(_data);
		_data = malloc(rs);
	}
	memcpy(_data, data, rs);
	return rs;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
