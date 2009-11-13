// SaveFile.cpp

#include "stdafx.h"
#include "SaveFile.h"

#include "gc/Object.h"

SaveFile::SaveFile(SafePtr<FS::Stream> &s, bool loading)
  : _stream(s)
  , _load(loading)
{
}

void SaveFile::RestoreAllLinks()
{
	SafePtr<void> boo; // to make compiler find GetRawPtr

	for( std::list<SafePtr<GC_Object>*>::iterator it = _refs.begin(); it != _refs.end(); ++it )
	{
		if( DWORD_PTR id = reinterpret_cast<DWORD_PTR>(GetRawPtr(**it)) )
		{
			IndexToPtr::iterator p = _indexToPtr.find(id);
			if( _indexToPtr.end() == p )
				throw std::runtime_error("ERROR: invalid links");
			SetRawPtr(**it, p->second);
			p->second->AddRef();
		}
		else
		{
			SetRawPtr(**it, NULL);
		}
	}
	_refs.clear();
}

void SaveFile::RegPointer(GC_Object *ptr, size_t index)
{
	assert(_ptrToIndex.empty());
	assert(0 == _indexToPtr.count(index));
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
