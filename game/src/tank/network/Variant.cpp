// Variant.cpp

#include "stdafx.h"
#include "Variant.h"

///////////////////////////////////////////////////////////////////////////////

DataStream::DataStream(bool serialize)
  : _serialization(serialize)
  , _entityLevel(0)
  , _ptr(_buffer.begin())
{
}

bool DataStream::Direction() const
{
	return _serialization;
}

void DataStream::Serialize(void *data, int bytes)
{
	_ASSERT(_entityLevel > 0); // everything must be inside an entity
	_ASSERT(bytes >= 0);
	if( _serialization )
	{
		_buffer.insert(_buffer.end(), (const char *) data, (const char *) data + bytes);
	}
	else
	{
		_ASSERT(bytes <= std::distance(_ptr, _buffer.end()));
		memcpy(data, &*_ptr, bytes);
		_ptr += bytes;
	}
}

void DataStream::EntityBegin()
{
	if( 0 == _entityLevel++ )
	{
		if( _serialization )
		{
			_entitySizeOffset = _buffer.size();
			_buffer.resize(_buffer.size() + sizeof(EntitySizeType)); // placeholder
		}
		else
		{
			_ASSERT(EntityProbe());
			_ptr += sizeof(EntitySizeType); // skip entity size
		}
	}
}

void DataStream::EntityEnd()
{
	_ASSERT(_entityLevel > 0);
	if( 0 == --_entityLevel )
	{
		if( _serialization )
		{
			size_t entitySize = _buffer.size() - _entitySizeOffset;
			_ASSERT(entitySize < 0xffff);
			*(EntitySizeType *) &_buffer[_entitySizeOffset] = (EntitySizeType) entitySize;
		}
		else
		{
			_buffer.erase(_buffer.begin(), _ptr);
			_ptr = _buffer.begin();
		}
	}
}

bool DataStream::EntityProbe() const
{
	_ASSERT(!_serialization);
	if( 0 == _entityLevel )
	{
		_ASSERT(_ptr <= _buffer.end());
		size_t restSize = (unsigned) std::distance(_ptr, const_cast<std::vector<char> &>(_buffer).end());
		if( restSize < sizeof(EntitySizeType) )
		{
			return false;
		}
		const EntitySizeType &entitySize = (const EntitySizeType &) *_ptr;
		return entitySize <= restSize;
	}
	return true;
}

bool DataStream::IsEmpty() const
{
	return _buffer.empty();
}

int DataStream::Send(SOCKET s)
{
	_ASSERT(_serialization);
	_ASSERT(0 == _entityLevel);

	if( _buffer.empty() )
	{
		return 0;
	}

	size_t sent = 0;
	do
	{
		int result = send(s, &_buffer.front() + sent, _buffer.size() - sent, 0);
		if( SOCKET_ERROR == result )
		{
			int err = WSAGetLastError();
			if( WSAEWOULDBLOCK != err )
			{
				return err;
			}
			break;
		}
		else
		{
			_ASSERT(result > 0);
			sent += result;
		}
	} while( sent < _buffer.size() );

	// remove sent bytes
	_ASSERT(sent <= _buffer.size());
	_buffer.erase(_buffer.begin(), _buffer.begin() + sent);
	return 0;
}

int DataStream::Recv(SOCKET s)
{
	_ASSERT(!_serialization);
	_ASSERT(0 == _entityLevel);
	_ASSERT(_buffer.begin() == _ptr);

	u_long pending = 0;
	if( ioctlsocket(s, FIONREAD, &pending) )
		return -1;

	if( 0 == pending )
		return 0;

	size_t offset = _buffer.size();
	_buffer.resize(_buffer.size() + pending);
	_ptr = _buffer.begin();

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


Variant::TypeId Variant::RegisterType(Constructor ctor, Destructor dtor, Serialize ser)
{
#ifdef VARIANT_DEBUG
	_ASSERT(_reg && !_init); // did you forget VARIANT_DECLARE_TYPE or Variant::Init()?
#endif
	UserType ut = {ctor, dtor, ser};
	_types.push_back(ut);
	return _types.size() - 1;
}

bool Variant::DeclareType(TypeId (*declarator)())
{
	_ASSERT(GetDecl().end() == std::find(GetDecl().begin(), GetDecl().end(), declarator));
	GetDecl().push_back(declarator);
	return true;
}

Variant::Declarators& Variant::GetDecl()
{
	static Declarators decl;
	return decl;
}


void Variant::Init()
{
#ifdef VARIANT_DEBUG
	_ASSERT(!_init); // did you call Init() more than once?
	_reg = true;
#endif
	for( size_t i = 0; i < GetDecl().size(); ++i )
	{
		GetDecl()[i] ();
	}
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
VARIANT_IMPLEMENT_TYPE(unsigned char) RAW
VARIANT_IMPLEMENT_TYPE(unsigned short) RAW
VARIANT_IMPLEMENT_TYPE(unsigned int) RAW
VARIANT_IMPLEMENT_TYPE(unsigned long) RAW
VARIANT_IMPLEMENT_TYPE(float) RAW
VARIANT_IMPLEMENT_TYPE(double) RAW

VARIANT_IMPLEMENT_TYPE(std::string)
{
	_ASSERT(value.length() < 0xffff);
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
