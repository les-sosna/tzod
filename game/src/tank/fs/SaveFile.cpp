// SaveFile.cpp

#include "stdafx.h"
#include "SaveFile.h"


bool SaveFile::RestoreAllLinks()
{
	SafePtr<void> boo; // to make compiler find GetRawPtr

	for( std::list<SafePtr<void>*>::iterator it = _refs.begin(); it != _refs.end(); ++it )
	{
		if( DWORD_PTR id = reinterpret_cast<DWORD_PTR>(GetRawPtr(**it)) )
		{
			IndexToPtr::iterator p = _indexToPtr.find(id);
            if( _indexToPtr.end() == p )
				return false;
			else
				SetRawPtr(**it, p->second);
		}
		else
		{
			SetRawPtr(**it, NULL);
		}
	}
	_refs.clear();
	return true;
}

void SaveFile::RegPointer(void *ptr, size_t index)
{
	_ASSERT(_ptrToIndex.empty());
	_ASSERT(0 == _indexToPtr.count(index));
	_indexToPtr[index] = ptr;
}

void SaveFile::Serialize(string_t &str)
{
	string_t::size_type len = str.length();
	Serialize(len);
	if( len )
	{
		if( loading() )
		{
			std::vector<string_t::value_type> buffer(len);
			SerializeArray(&*buffer.begin(), len);
			str.resize(0);
			str.reserve(len);
			str.insert(str.begin(), buffer.begin(), buffer.end());
		}
		else
		{
			SerializeArray(const_cast<string_t::value_type*>(str.data()), len);
		}
	}
}


// end of file
