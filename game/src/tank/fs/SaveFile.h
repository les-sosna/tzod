// SaveFile.h

#pragma once

#include "FileSystem.h"

class GC_Object;

class SaveFile
{
	typedef std::map<GC_Object*, size_t> PtrToIndex;
	typedef std::map<size_t, GC_Object*> IndexToPtr;

	PtrToIndex _ptrToIndex;
	IndexToPtr _indexToPtr;

	std::list<SafePtr<GC_Object>*> _refs;

	SafePtr<FS::Stream> _stream;
	bool _load;

public:
	SaveFile(SafePtr<FS::Stream> &s, bool loading);

	bool loading() const
	{
		return _load;
	}


	void Serialize(string_t &str);

	template<class T>
	void Serialize(T &obj);
	template<class T>
	void Serialize(const T &obj);

	template<class T>
	void Serialize(SafePtr<T> &ptr);
	template<class T>
	void Serialize(const SafePtr<T> &ptr);

	template<class T>
	void SerializeArray(T *p, size_t count);

	void RestoreAllLinks(); // throws std::runtime_error
	void RegPointer(GC_Object *ptr, size_t index);

private:
	template<class T>
	void Serialize(T *) {assert(!"you are not allowed to serialize raw pointers");}
};

template<class T>
void SaveFile::Serialize(T &obj)
{
	assert(0 != strcmp(typeid(obj).raw_name(), typeid(string_t).raw_name()));
	assert(NULL == strstr(typeid(obj).raw_name(), "SafePtr"));
	if( loading() )
		_stream->Read(&obj, sizeof(T));
	else
		_stream->Write(&obj, sizeof(T));
}

template<class T>
void SaveFile::Serialize(const T &obj)
{
	assert(!loading());
	assert(0 != strcmp(typeid(obj).raw_name(), typeid(string_t).raw_name()));
	assert(NULL == strstr(typeid(obj).raw_name(), "SafePtr"));
	_stream->Write(&obj, sizeof(T));
}

template<class T>
void SaveFile::Serialize(SafePtr<T> &ptr)
{
	DWORD_PTR id;
	if( loading() )
	{
		Serialize(id);
		if( id )
		{
			SetRawPtr(ptr, reinterpret_cast<T*>(id));
			_refs.push_back(reinterpret_cast<SafePtr<GC_Object>*>(&ptr));
		}
		else
		{
			ptr = NULL;
		}
	}
	else
	{
		if( !ptr || ptr->IsKilled() )
		{
			id = 0;
		}
		else
		{
			if( _ptrToIndex.count(GetRawPtr(ptr)) )
			{
				id = _ptrToIndex[GetRawPtr(ptr)];
			}
			else
			{
				id = _ptrToIndex.size() + 1;
				_ptrToIndex[GetRawPtr(ptr)] = id;
			}
		}
		Serialize(id);
	}
}

template<class T>
void SaveFile::Serialize(const SafePtr<T> &ptr)
{
	assert(!loading());
	DWORD_PTR id;
	if( !ptr )
	{
		id = 0;
	}
	else
	{
		if( _ptrToIndex.count(GetRawPtr(ptr)) )
		{
			id = _ptrToIndex[GetRawPtr(ptr)];
		}
		else
		{
			id = _ptrToIndex.size() + 1;
			_ptrToIndex[GetRawPtr(ptr)] = id;
		}
	}
	Serialize(id);
}

template<class T>
void SaveFile::SerializeArray(T *p, size_t count)
{
	assert(0 != strcmp(typeid(T).raw_name(), typeid(string_t).raw_name()));
	assert(NULL == strstr(typeid(T).raw_name(), "SafePtr"));
	if( loading() )
		_stream->Read(p, sizeof(T) * count);
	else
		_stream->Write(p, sizeof(T) * count);
}

// end of file
