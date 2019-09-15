#include "inc/fsmem/FileSystemMemory.h"
#include <cassert>

using namespace FS;

size_t MemoryStream::Read(void *dst, size_t size, size_t count)
{
	size_t bytes = size * count;
	if (_streamPos + bytes > _data.size())
		return 0;
	memcpy(dst, &_data[_streamPos], bytes);
	_streamPos += bytes;
	return count;
}

void MemoryStream::Write(const void *src, size_t size)
{
	if( _data.size() - _streamPos < size )
		_data.resize(_streamPos + size);
	memcpy(&_data[_streamPos], src, size);
	_streamPos += size;
}

void MemoryStream::Seek(long long amount, unsigned int origin)
{
	switch (origin)
	{
	case SEEK_SET:
		assert(amount >= 0 && amount <= _data.size());
		_streamPos = static_cast<size_t>(amount);
		break;
	case SEEK_CUR:
		assert((long long)_streamPos + amount >= 0 && (long long)_streamPos + amount <= _data.size());
		_streamPos += static_cast<size_t>(amount);
		break;
	case SEEK_END:
		assert(amount <= 0 && (long long)_streamPos + amount >= 0);
		_streamPos = _data.size() + static_cast<size_t>(amount);
		break;
	default:
		assert(false);
	}
}

long long MemoryStream::Tell() const
{
	return (long long)_streamPos;
}

