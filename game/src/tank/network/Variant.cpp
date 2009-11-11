// Variant.cpp

#include "stdafx.h"
#include "Variant.h"

///////////////////////////////////////////////////////////////////////////////

static Bytef g_buffer[16384];

DataStream::DataStream(bool serialize)
  : _serialization(serialize)
  , _entityLevel(0)
{
	memset(&_z, 0, sizeof(z_stream));
	int result;
	if( _serialization )
	{
		result = deflateInit(&_z, Z_BEST_COMPRESSION);
	}
	else
	{
		result = inflateInit(&_z);
	}
	if( Z_OK != result )
	{
		throw std::runtime_error("failed to init zlib");
	}
}

DataStream::~DataStream()
{
	if( _serialization )
	{
		deflateEnd(&_z);
	}
	else
	{
		inflateEnd(&_z);
	}
}

size_t DataStream::GetTraffic() const
{
	return _serialization ? _z.total_out : _z.total_in;
}

bool DataStream::Direction() const
{
	return _serialization;
}

void DataStream::Serialize(void *data, int bytes)
{
	assert(_entityLevel > 0); // everything must be inside an entity
	assert(bytes >= 0);
	if( _serialization )
	{
		_z.avail_in = bytes;
		_z.next_in = (Bytef *) data;
		do 
		{
			_z.next_out = g_buffer;
			_z.avail_out = sizeof(g_buffer);
			deflate(&_z, Z_NO_FLUSH);
			_buffer.insert(_buffer.end(), g_buffer, _z.next_out);
		} while( _z.avail_in || 0 == _z.avail_out );
	}
	else
	{
		assert(_z.avail_in);
		_z.next_out = (Bytef *) data;
		_z.avail_out = bytes;
		int result = inflate(&_z, Z_SYNC_FLUSH);
		assert(Z_OK == result);
		assert(0 == _z.avail_out);
	}
}

void DataStream::EntityBegin()
{
	++_entityLevel;
	if( _serialization )
	{
		if( 1 == _entityLevel )
		{
			_entitySizeOffset = _buffer.size();
			_buffer.resize(_buffer.size() + sizeof(EntitySizeType)); // placeholder for size
		}
	}
	else
	{
		assert(EntityProbe());
		if( 1 == _entityLevel )
		{
			_z.avail_in = *(EntitySizeType *) &_buffer[0] - sizeof(EntitySizeType);
			_z.next_in = (Bytef *) &_buffer[sizeof(EntitySizeType)];
		}
	}
}

void DataStream::EntityEnd()
{
	assert(_entityLevel > 0);
	if( 0 == --_entityLevel )
	{
		if( _serialization )
		{
			assert(0 == _z.avail_in);
			_z.next_in = NULL;
			do 
			{
				_z.next_out = g_buffer;
				_z.avail_out = sizeof(g_buffer);
				deflate(&_z, Z_SYNC_FLUSH);
				_buffer.insert(_buffer.end(), g_buffer, _z.next_out);
			} while( 0 == _z.avail_out );

			size_t entitySize = _buffer.size() - _entitySizeOffset;
			assert(entitySize < 0xffff);
			*(EntitySizeType *) &_buffer[_entitySizeOffset] = (EntitySizeType) entitySize;
		}
		else
		{
			assert(0 == _z.avail_in);
			_buffer.erase(_buffer.begin(), _buffer.begin() + *(EntitySizeType *) &_buffer[0]);
		}
	}
}

bool DataStream::EntityProbe() const
{
	assert(!_serialization);
	if( 0 == _entityLevel )
	{
		if( _buffer.size() < sizeof(EntitySizeType) )
		{
			return false;
		}
		return _buffer.size() >= *(EntitySizeType *) &_buffer[0];
	}
	return true;
}

bool DataStream::IsEmpty() const
{
	return _buffer.empty();
}

int DataStream::Send(SOCKET s, size_t *outSent)
{
	assert(_serialization);
	assert(0 == _entityLevel);

	size_t sent = 0;

	if( !_buffer.empty() )
	{
		do
		{
			int result = send(s, &_buffer.front() + sent, _buffer.size() - sent, 0);
			if( SOCKET_ERROR == result )
			{
				return WSAGetLastError();
			}
			else
			{
				assert(result > 0);
				sent += result;
			}
		} while( sent < _buffer.size() );

		// remove sent bytes
		assert(sent <= _buffer.size());
		_buffer.erase(_buffer.begin(), _buffer.begin() + sent);
	}

	if( outSent )
	{
		*outSent = sent;
	}

	return 0;
}

int DataStream::Recv(SOCKET s)
{
	assert(!_serialization);
	assert(0 == _entityLevel);

	u_long pending = 0;
	if( ioctlsocket(s, FIONREAD, &pending) )
		return -1;

	if( 0 == pending )
		return 0;

	size_t offset = _buffer.size();
	_buffer.resize(_buffer.size() + pending);

	return recv(s, &_buffer[offset], pending, 0);
}

///////////////////////////////////////////////////////////////////////////////
// Variant static members

std::vector<Variant::UserType> Variant::_types;

#ifdef VARIANT_DEBUG
bool Variant::_reg = false;
bool Variant::_init = false;
#endif

///////////////////////////////////////////////////////////////////////////////

Variant::Variant()
  : _data(NULL)
  , _type(-1)
{
}

Variant::Variant(const Variant &src)
  : _type(src._type)
  , _data(src._data ? _types[src._type].ctor(src._data) : NULL)
{
}

Variant::Variant(TypeId type, const void *copyFrom)
  : _type(type)
  , _data(_types[type].ctor(copyFrom))
{
}

Variant::~Variant()
{
	Clear();
}

void Variant::Clear()
{
	if( _data )
	{
		_types[_type].dtor(_data);
		_data = NULL;
		_type = -1;
	}
}

Variant& Variant::operator = (const Variant &src)
{
	Clear();
	_data = _types[src._type].ctor(src._data);
	_type = src._type;
	return *this;
}

void Variant::ChangeType(TypeId type)
{
	Clear();
	_data = _types[type].ctor(NULL);
	_type = type;
}

void Variant::ScheduleTypeRegistration(CtorType ctor, DtorType dtor, Serialize ser, TypeId *result)
{
	UserType ut = {ctor, dtor, ser};
	GetPendingRegs().push_back(RegData(result, ut));
}

Variant::PendingRegList& Variant::GetPendingRegs()
{
	static PendingRegList decl;
	return decl;
}


void Variant::Init()
{
#ifdef VARIANT_DEBUG
	assert(!_init); // did you call Init() more than once?
	_reg = true;
#endif
	for( PendingRegList::const_iterator it = GetPendingRegs().begin(); it != GetPendingRegs().end(); ++it )
	{
		_types.push_back(it->second);
		*it->first = _types.size() - 1;
	}
	GetPendingRegs().clear();
#ifdef VARIANT_DEBUG
	_reg = false;
	_init = true;
#endif
}

///////////////////////////////////////////////////////////////////////////////
// basic types

VARIANT_IMPLEMENT_TYPE(bool) RAW
VARIANT_IMPLEMENT_TYPE(char) RAW
VARIANT_IMPLEMENT_TYPE(short) RAW
VARIANT_IMPLEMENT_TYPE(int) RAW
VARIANT_IMPLEMENT_TYPE(long) RAW
VARIANT_IMPLEMENT_TYPE(long long) RAW
VARIANT_IMPLEMENT_TYPE(unsigned char) RAW
VARIANT_IMPLEMENT_TYPE(unsigned short) RAW
VARIANT_IMPLEMENT_TYPE(unsigned int) RAW
VARIANT_IMPLEMENT_TYPE(unsigned long) RAW
VARIANT_IMPLEMENT_TYPE(unsigned long long) RAW
VARIANT_IMPLEMENT_TYPE(float) RAW
VARIANT_IMPLEMENT_TYPE(double) RAW

VARIANT_IMPLEMENT_TYPE(std::string)
{
	assert(value.length() < 0xffff);
	unsigned short len = value.length();
	s & len;
	value.resize(len);
	for( size_t i = 0; i < value.length(); ++i )
	{
		s & value[i];
	}
	return s;
}


// end of file
