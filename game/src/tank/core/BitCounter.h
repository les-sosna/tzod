// BitCounter.h

#pragma once

template <unsigned int capacity>
class BitCounter
{
	typedef unsigned int StorageType;
	StorageType _data[capacity / (sizeof(StorageType) * 8)];
	StorageType _current;
public:
	BitCounter() : _current(0)
	{
		for( unsigned int i = 0; i < sizeof(_data) / sizeof(_data[0]); ++i )
		{
			_data[i] = 0;
		}
	}

	void Push(bool value)
	{
		StorageType mask = StorageType(1) << _current;
		unsigned int c = _current / (sizeof(StorageType) * 8);
#pragma warning (push)
#pragma warning (disable: 4804) // unsafe use of type 'bool' in operation
#if 1
		_data[c] = (_data[c] & ~mask) | (-value & mask); // superscalar version
#else
		_data[c] ^= (-value ^ _data[c]) & mask;
#endif
#pragma warning (pop)
		++_current;
		_current %= capacity;
	}

	unsigned int Count() const
	{
		unsigned int c = 0;
		for( unsigned int i = 0; i < sizeof(_data) / sizeof(_data[0]); ++i )
		{
			unsigned int v = _data[i] - ((_data[i] >> 1) & 0x55555555);
			v = (v & 0x33333333) + ((v >> 2) & 0x33333333);      // temp
			c += ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
		}
		return c;
	}

	float GetCapacity() const
	{
		return capacity;
	}
};


// end of file
