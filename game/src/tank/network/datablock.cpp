// datablock.cpp
///////////////////////////////////////////////////

#include "stdafx.h"
#include "datablock.h"

////////////////////////////////////////////////////////////////

DataBlock::DataBlock()
{
	_data   = malloc(_raw_size = sizeof(_header) );
	h().type = DBTYPE_UNKNOWN;
	h().size = 0;
}

DataBlock::DataBlock(size_type size)
{
	_data   = malloc(_raw_size = sizeof(_header) + size);
	h().type = DBTYPE_UNKNOWN;
	h().size = size;
}

DataBlock::DataBlock(const DataBlock &src)
{
	_data = malloc(_raw_size = src._raw_size);
	memcpy(_data, src._data, _raw_size);
}

DataBlock::~DataBlock()
{
	_ASSERT(_data);
	free(_data);
}

DataBlock& DataBlock::operator=(const DataBlock &src)
{
	_ASSERT(_data);
	if( src._raw_size != _raw_size )
	{
		free(_data);
		_data = malloc(_raw_size = src._raw_size);
	}
	memcpy(_data, src._data, _raw_size);
	return *this;
}

bool DataBlock::from_stream(void **pointer, size_t *stream_len)
{
	if( *stream_len < sizeof(_header) )
		return false;
	size_t raw_size = ((_header*)(*pointer))->size + sizeof(_header);
	if( *stream_len < raw_size )
		return false;
	if( raw_size != _raw_size )
	{
		free(_data);
		_data = malloc(_raw_size = raw_size);
	}
	memcpy(_data, (*pointer), _raw_size);
	( char*& ) (*pointer) += raw_size;
	*stream_len -= raw_size;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
